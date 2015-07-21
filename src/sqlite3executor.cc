/*
  sqlite3executor.cc

  Qore Programming Language

  Copyright 2003 - 2021 Qore Technologies, s.r.o <http://qore.org>

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
        m_realArgs(new QoreListNode(autoTypeInfo), xsink) {
}

QoreSqlite3Executor::~QoreSqlite3Executor() {
}

QoreValue QoreSqlite3Executor::columnValue(sqlite3_stmt * stmt, int index) {
    int columnType = sqlite3_column_type(stmt, index);

    switch (columnType) {
    case SQLITE_INTEGER:
        return sqlite3_column_int(stmt, index);

    case SQLITE_FLOAT:
        return sqlite3_column_double(stmt, index);

    case SQLITE_BLOB: {
        int nBlob = sqlite3_column_bytes(stmt, index);
        unsigned char *zBlob = (unsigned char *)malloc(nBlob);
        memcpy(zBlob, sqlite3_column_blob(stmt, index), nBlob);
        return new BinaryNode(zBlob, nBlob);
    }

    case SQLITE_NULL:
        return null();

    case SQLITE_TEXT:
    default:
        return new QoreStringNode((const char*)sqlite3_column_text(stmt, index));
        break;
    };

    return QoreValue();
}

QoreValue QoreSqlite3Executor::exec(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink) {
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, args, true, "DBI:SQLITE3:EXEC", xsink)), xsink);
    if (*xsink) {
        return QoreValue();
    }

    if (hash->size() > 0) {
        return hash.release();
    }

    return sqlite3_changes(m_handler);
}

QoreValue QoreSqlite3Executor::execRaw(
    Datasource *ds,
    const QoreString *qstr,
    ExceptionSink *xsink) {
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, 0, false, "DBI:SQLITE3:EXECRAW", xsink)), xsink);
    if (*xsink) {
        return QoreValue();
    }

    if (hash->size() > 0) {
        return hash.release();
    }

    return sqlite3_changes(m_handler);
}

QoreValue QoreSqlite3Executor::select(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink) {
    ReferenceHolder<QoreHashNode> hash(reinterpret_cast<QoreHashNode*>(select_internal(ds, qstr, args, true, "DBI:SQLITE3:SELECT", xsink)), xsink);
    if (*xsink) {
        return QoreValue();
    }

    if (hash->size() > 0) {
        return hash.release();
    }

    return sqlite3_changes(m_handler);
}

QoreListNode* QoreSqlite3Executor::select_rows(
    Datasource *ds,
    const QoreString *qstr,
    const QoreListNode *args,
    ExceptionSink *xsink) {
    QoreString statement(qstr);
    if (!parseForBind(statement, args, xsink)) {
        xsink->raiseException("DBI:SQLITE3:SELECT_ROWS", "failed to parse bind variables");
        return nullptr;
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(m_handler, statement.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        xsink->raiseException("DBI:SQLITE3:SELECT_ROWS", "sqlite3 error: %s", sqlite3_errmsg(m_handler));
        return nullptr;
    }

    if (!bindParameters(stmt, xsink)) {
        xsink->raiseException("DBI:SQLITE3:SELECT_ROWS", "failed to bind variables");
        return nullptr;
    }

    ReferenceHolder<QoreListNode> res(new QoreListNode(autoTypeInfo), xsink);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QoreHashNode* head = new QoreHashNode(autoTypeInfo);

        for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
            head->setKeyValue(sqlite3_column_name(stmt, i), columnValue(stmt, i), xsink);
        }
        res->push(head, xsink);
    }

    sqlite3_finalize(stmt);
    return res.release();
}

bool QoreSqlite3Executor::parseForBind(QoreString & str, const QoreListNode* args, ExceptionSink *xsink) {
    char quote = 0;
    const char *p = str.c_str();
    QoreString tmp;
    int index = 0;
    int nParams = 0;
    int nIterations = 0;
    size_t argsSize = (args) ? args->size() : 0;

    while (*p) {
        if (!quote && (*p) != '%') {
            ++p;
            continue;
        }
        // found value marker

        int offset = p - str.c_str();

        p++;
        const QoreValue v = args ? args->retrieveEntry(index++) : QoreValue();
        ++nIterations;

        if ((*p) == 'd') {
            DBI_concat_numeric(&tmp, v);
            str.replace(offset, 2, tmp.c_str());
            p = str.c_str() + offset + tmp.strlen();
            tmp.clear();
            continue;
        }
        if ((*p) == 's') {
            if (DBI_concat_string(&tmp, v, xsink))
                return false;
            str.replace(offset, 2, tmp.c_str());
            p = str.c_str() + offset + tmp.strlen();
            tmp.clear();
            continue;
        }
        if ((*p) != 'v') {
            xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                                  "invalid value specification (expecting '%v' or '%%d', got %%%c)", *p);
            return false;
        }
        ++p;
        if (isalpha(*p)) {
            xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                                  "invalid value specification (expecting '%v' or '%%d', got %%v%c*)", *p);
            return false;
        }

        // replace value marker with "?<num>" - sqlite3 binding
        // find byte offset in case string buffer is reallocated with replace()
        ++nParams;
        tmp.sprintf("?%d", nParams);
        str.replace(offset, 2, tmp.c_str());
        p = str.c_str() + offset + tmp.strlen();
        tmp.clear();

        m_realArgs->push(v.refSelf(), xsink);

        if (((*p) == '\'') || ((*p) == '\"')) {
            if (!quote)
                quote = *p;
            else if (v)
                quote = '\0';
            ++p;
        }
    }

    if (nIterations != argsSize) {
        xsink->raiseException("DBI-EXEC-PARSE-EXCEPTION",
                              "Parameter count mismatch (args: %d ; processed: %d)", argsSize, nIterations);
        return false;
    }

    return true;
}

bool QoreSqlite3Executor::bindParameters(sqlite3_stmt * stmt, ExceptionSink *xsink) {
    qore_type_t argType;
    QoreValue arg;

    for (int i = 0; i < sqlite3_bind_parameter_count(stmt); ++i)     {
        arg = m_realArgs->retrieveEntry(i);
        argType = arg.getType();

        switch (argType) {
            case NT_NOTHING:
            case NT_NULL:
                if (SQLITE_OK != sqlite3_bind_null(stmt, i+1)) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind NULL");
                    return false;
                }
                break;
            case NT_INT:
                if (SQLITE_OK != sqlite3_bind_int64(stmt, i+1, arg.getAsBigInt())) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind integer");
                    return false;
                }
                break;
            case NT_FLOAT:
                if (SQLITE_OK != sqlite3_bind_double(stmt, i+1, arg.getAsFloat())) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind double/float");
                    return false;
                }
                break;
            case NT_STRING: {
                const QoreStringNode* s = arg.get<const QoreStringNode>();
                if (SQLITE_OK != sqlite3_bind_text(stmt, i+1, s->c_str(), s->strlen(), SQLITE_TRANSIENT)) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind string");
                    return false;
                }
                break;
            }
            case NT_BOOLEAN:
                if (SQLITE_OK != sqlite3_bind_double(stmt, i+1, arg.getAsBool())) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind bool");
                    return false;
                }
                break;
//         case NT_DATE:
//             // TODO/FIXME: date
//             xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Date Time handling is not implemented yet");
//             return false;
            case NT_BINARY: {
                const BinaryNode* b = arg.get<const BinaryNode>();
                if (SQLITE_OK != sqlite3_bind_blob(stmt, i+1, b->getPtr(), b->size(), nullptr)) {
                    xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind BLOB");
                    return false;
                }
                break;
            }
            default:
                xsink->raiseException("DBI-EXEC-BIND-EXCEPTION", "Cannot bind unsupported type '%s'",
                    arg.getTypeName());
                return false;
        } // switch
    } // for
    return true;
}

QoreHashNode* QoreSqlite3Executor::select_internal(
            Datasource *ds,
            const QoreString *qstr,
            const QoreListNode *args,
            bool binding,
            const char * calltype,
            ExceptionSink *xsink) {
    QoreString statement(qstr);
    if (binding) {
        if (!parseForBind(statement, args, xsink)) {
            xsink->raiseException(calltype, "failed to parse bind variables");
            return nullptr;
        }
    }

    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(m_handler, statement.c_str(), -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        xsink->raiseException(calltype, "sqlite3 error: %s", sqlite3_errmsg(m_handler));
        return nullptr;
    }

    if (binding) {
        if (!bindParameters(stmt, xsink)) {
            xsink->raiseException(calltype, "failed to bind variables");
            return nullptr;
        }
    }

    // columns as keys
    ReferenceHolder<QoreHashNode> hash(new QoreHashNode(autoTypeInfo), xsink);

    for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
        hash->setKeyValue(sqlite3_column_name(stmt, i), new QoreListNode(autoTypeInfo), xsink);
    }

    // fetch the results
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int i = 0; i < sqlite3_column_count(stmt); ++i) {
            QoreListNode* node = hash->getKeyValue(sqlite3_column_name(stmt, i)).get<QoreListNode>();
            node->push(columnValue(stmt, i), xsink);
        }
    }

    sqlite3_finalize(stmt);
    return hash.release();
}
