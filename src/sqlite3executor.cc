/*
  sqlite3executor.cc

  Qore Programming Language

  Copyright 2003 - 2009 Qore Technologies, s.r.o <http://qore.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "sqlite3executor.h"


QoreSqlite3Executor::QoreSqlite3Executor(sqlite3 * handle,
                                          ExceptionSink *xsink)
        : m_handler(handle),
        m_realArgs(new QoreListNode(), xsink)
{
}

QoreSqlite3Executor::~QoreSqlite3Executor()
{
}

AbstractQoreNode * QoreSqlite3Executor::columnValue(sqlite3_stmt * stmt, int index)
{
    int columnType = sqlite3_column_type(stmt, index);

    switch (columnType)
    {
    case SQLITE_INTEGER:
        return new QoreBigIntNode(sqlite3_column_int(stmt, index));
        break;
    case SQLITE_FLOAT:
        return new QoreFloatNode(sqlite3_column_double(stmt, index));
        break;
    case SQLITE_BLOB:
    {
        int nBlob = sqlite3_column_bytes(stmt, index);
        unsigned char *zBlob = (unsigned char *)malloc(nBlob);
        memcpy(zBlob, sqlite3_column_blob(stmt, index), nBlob);
        BinaryNode * bn = new BinaryNode(zBlob, nBlob);
        return bn;
        break;
    }
    case SQLITE_NULL:
        return null();
        break;
    case SQLITE_TEXT:
    default:
        return new QoreStringNode((const char*)sqlite3_column_text(stmt, index));
        break;
    };
    return 0;
}

AbstractQoreNode * QoreSqlite3Executor::exec(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink)
{
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, args, true, xsink)), xsink);
    if (*xsink)
        return 0;

    if (hash->size() > 0)
        return hash.release();

    return new QoreBigIntNode(sqlite3_changes(m_handler));
}

#ifdef _QORE_HAS_DBI_EXECRAW
AbstractQoreNode * QoreSqlite3Executor::execRaw(
    Datasource *ds,
    const QoreString *qstr,
    ExceptionSink *xsink)
{
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, 0, false, xsink)), xsink);
    if (*xsink)
        return 0;

    if (hash->size() > 0)
        return hash.release();

    return new QoreBigIntNode(sqlite3_changes(m_handler));
}
#endif
            
AbstractQoreNode * QoreSqlite3Executor::select(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink)
{
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, args, true, xsink)), xsink);
    if (*xsink)
        return 0;

    if (hash->size() > 0)
        return hash.release();

    return new QoreBigIntNode(sqlite3_changes(m_handler));
}

AbstractQoreNode * QoreSqlite3Executor::select_rows(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink)
{
    QoreString statement(qstr);
    if (!parseForBind(statement, args, xsink))
        return xsink->raiseException("DBI:SQLITE3:SELECT_ROWS",
                                      "failed to parse bind variables");

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_handler, statement.getBuffer(), -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return xsink->raiseException("DBI:SQLITE3:SELECT_ROWS",
                                      "sqlite3 error: %s", sqlite3_errmsg(m_handler));

    if (!bindParameters(stmt, xsink))
        return xsink->raiseException("DBI:SQLITE3:SELECT_ROWS",
                                      "failed to bind variables");

    ReferenceHolder<QoreListNode> res(new QoreListNode(), xsink);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        QoreHashNode * head = new QoreHashNode();

        for (int i = 0; i < sqlite3_column_count(stmt); ++i)
        {
            head->setKeyValue(sqlite3_column_name(stmt, i),
                              columnValue(stmt, i),
                              xsink);
        }
        res->push(head);
    }

    sqlite3_finalize(stmt);
    return res.release();
}

bool QoreSqlite3Executor::parseForBind(QoreString & str,
                                        const QoreListNode *args,
                                        ExceptionSink *xsink)
{
    char quote = 0;
    const char *p = str.getBuffer();
    QoreString tmp;
    int index = 0;
    int nParams = 0;
    int nIterations = 0;
    int argsSize = (args) ? args->size() : 0;

//     m_realArgs = new QoreListNode();

    while (*p)
    {
        if (!quote && (*p) != '%')
        {
            ++p;
            continue;
        }
        // found value marker

        int offset = p - str.getBuffer();

        p++;
        const AbstractQoreNode *v = args ? args->retrieve_entry(index++) : NULL;
        ++nIterations;
        if (v == 0)
        {
            xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                              "Parameter count mismatch (args: %d ; processed: %d)", argsSize, nIterations);
            return false;
        }

        if ((*p) == 'd')
        {
            DBI_concat_numeric(&tmp, v);
            str.replace(offset, 2, &tmp);
            p = str.getBuffer() + offset + tmp.strlen();
            tmp.clear();
            continue;
        }
        if ((*p) == 's')
        {
            if (DBI_concat_string(&tmp, v, xsink))
                return false;
            str.replace(offset, 2, &tmp);
            p = str.getBuffer() + offset + tmp.strlen();
            tmp.clear();
            continue;
        }
        if ((*p) != 'v')
        {
            xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                                  "invalid value specification (expecting '%v' or '%%d', got %%%c)", *p);
            return false;
        }
        ++p;
        if (isalpha(*p))
        {
            xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                                  "invalid value specification (expecting '%v' or '%%d', got %%v%c*)", *p);
            return false;
        }

        // replace value marker with "?<num>" - sqlite3 binding
        // find byte offset in case string buffer is reallocated with replace()
        ++nParams;
        tmp.sprintf("?%d", nParams);
        str.replace(offset, 2, &tmp);
        p = str.getBuffer() + offset + tmp.strlen();
        tmp.clear();

        m_realArgs->push(v->realCopy());

        if (((*p) == '\'') || ((*p) == '\"'))
        {
            if (!quote)
                quote = *p;
            else if (v)
                quote = 0;
            ++p;
        }
    }

    if (nIterations != argsSize)
    {
        xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                              "Parameter count mismatch (args: %d ; processed: %d)", argsSize, nIterations);
        return false;
    }

    return true;
}

bool QoreSqlite3Executor::bindParameters(sqlite3_stmt * stmt, ExceptionSink *xsink)
{
    qore_type_t argType;
    const AbstractQoreNode * arg;

    for (int i = 0; i < sqlite3_bind_parameter_count(stmt); ++i)
    {
        arg = m_realArgs->retrieve_entry(i);

        if (!arg)
        {
            xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                   "unknown/missing argument to bind.");
            return false;
        }

        argType = arg->getType();

        switch (argType)
        {
        case NT_INT:
            if (SQLITE_OK != sqlite3_bind_int64(stmt, i+1, reinterpret_cast<const QoreBigIntNode*>(arg)->val))
            {
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                       "Cannot bind integer.");
                return false;
            }
            break;
        case NT_FLOAT:
            if (SQLITE_OK != sqlite3_bind_double(stmt, i+1, reinterpret_cast<const QoreFloatNode*>(arg)->f))
            {
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                       "Cannot bind double/fload.");
                return false;
            }
            break;
        case NT_STRING:
        {
            bool del;
            QoreString s(reinterpret_cast<const QoreStringNode*>(arg)->getStringRepresentation(del));
            if (SQLITE_OK != sqlite3_bind_text(stmt, i+1, s.getBuffer(), s.strlen(), SQLITE_TRANSIENT))
            {
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                       "Cannot bind string.");
                return false;
            }
            break;
        }
        case NT_BOOLEAN:
            if (SQLITE_OK != sqlite3_bind_double(stmt, i+1, reinterpret_cast<const QoreBoolNode*>(arg)->getValue()))
            {
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                       "Cannot bind double/fload.");
                return false;
            }
            break;
//         case NT_DATE:
//             // TODO/FIXME: date
//             xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Date Time handling is not implemented yet");
//             return false;
        case NT_BINARY:
        {
            const BinaryNode * b = reinterpret_cast<const BinaryNode*>(arg);
            if (SQLITE_OK != sqlite3_bind_blob(stmt, i+1, b->getPtr(), b->size(), NULL))
            {
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                       "Cannot bind BLOB.");
                return false;
            }
            break;
        }
        default:
            xsink->raiseException("DBI-EXEC-BIND-EXCEPTION",
                                  "Cannot bind '%s' for Sqlite3 by its design.", arg->getTypeName());
            return false;
        } // switch
    } // for
    return true;
}

AbstractQoreNode * QoreSqlite3Executor::select_internal(
            Datasource *ds,
            const QoreString *qstr,
            const QoreListNode *args,
            bool binding,
            ExceptionSink *xsink)
{
    QoreString statement(qstr);//new QoreString(qstr);
    if (binding) {
        if (!parseForBind(statement, args, xsink))
            return xsink->raiseException("DBI:SQLITE3:SELECT",
                                         "failed to parse bind variables");
    }

    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(m_handler, statement.getBuffer(), -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return xsink->raiseException("DBI:SQLITE3:SELECT",
                                      "sqlite3 error: %s", sqlite3_errmsg(m_handler));

    if (binding) {
        if (!bindParameters(stmt, xsink))
            return xsink->raiseException("DBI:SQLITE3:SELECT",
                                         "failed to bind variables");
    }

    // columns as keys
    ReferenceHolder<QoreHashNode> hash(new QoreHashNode(), xsink);

    for (int i = 0; i < sqlite3_column_count(stmt); ++i)
        hash->setKeyValue(sqlite3_column_name(stmt, i), new QoreListNode(), xsink);

    // fetch the results
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        for (int i = 0; i < sqlite3_column_count(stmt); ++i)
        {
            QoreListNode * node = reinterpret_cast<QoreListNode *>(hash->getKeyValue(sqlite3_column_name(stmt, i)));
            node->push(columnValue(stmt, i));
        }
    }

    sqlite3_finalize(stmt);
    return hash.release();
}
