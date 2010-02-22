/*
  sqlite3executor.h

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

#ifndef SQLITE3EXECUTOR_H
#define SQLITE3EXECUTOR_H

#include <sqlite3.h>
#include <qore/Qore.h>


/*! \brief Data operations class.
It will be created for every request from sqlite3module functions
to keep statement related data clean and separated from QoreSqlite3Connection.
\author Petr Vanek <petr.vanek@qoretechnologies.com>
*/
class QoreSqlite3Executor
{
    public:
        QoreSqlite3Executor(sqlite3 * handler, ExceptionSink *xsink);
        ~QoreSqlite3Executor();

        /*! \brief Implementation for Qore DB API exec().
        It's primarily used for DDL/INSERT/UPDATE/DELETE statemets, but
        it can handle all stuff as it's calling select() method.
        \param ds a Datasource reference from Qore API.
        \param qstr a SQL statement from Qore API.
        \param args a list with bindable parameters from Qore API.
        \param xsink exception handler.
        \retval AbstractQoreNode* hash for SELECTs, count of the affected
                rows for CRUD stuff.
        */
        AbstractQoreNode * exec(Datasource *ds,
                                const QoreString *qstr,
                                const QoreListNode *args,
                                ExceptionSink *xsink);

#ifdef _QORE_HAS_DBI_EXECRAW
        /*! \brief Implementation for Qore DB API execRaw().
        It's primarily used for DDL statemets, but
        it can handle all stuff. But there is no variable binding.
        \param ds a Datasource reference from Qore API.
        \param qstr a SQL statement from Qore API.
        \param xsink exception handler.
        \retval AbstractQoreNode* hash for SELECTs, count of the affected
                rows for CRUD stuff.
        */
        AbstractQoreNode * execRaw(Datasource *ds,
                                const QoreString *qstr,
                                ExceptionSink *xsink);
#endif

        /*! \brief Implementation for Qore DB API select().
        \param ds a Datasource reference from Qore API.
        \param qstr a SQL statement from Qore API.
        \param args a list with bindable parameters from Qore API.
        \param xsink exception handler.
        \retval AbstractQoreNode* QoreHashNode with coluns as keys, data as lists.
        */
        AbstractQoreNode * select(
            Datasource *ds,
            const QoreString *qstr,
            const QoreListNode *args,
            ExceptionSink *xsink);

        /*! \brief Implementation for Qore DB API selectRows().
        \param ds a Datasource reference from Qore API.
        \param qstr a SQL statement from Qore API.
        \param args a list with bindable parameters from Qore API.
        \param xsink exception handler.
        \retval AbstractQoreNode* QoreListNode with the list of hashes.
        */
        AbstractQoreNode * select_rows(
            Datasource *ds,
            const QoreString *qstr,
            const QoreListNode *args,
            ExceptionSink *xsink);

    private:
        //! Everywhere used current sqlite3 connection.
        sqlite3 * m_handler;
        //! Really used parameters list for statement (%s etc. filtered out)
        ReferenceHolder<QoreListNode> m_realArgs;

        /*! \brief Universal Sqlite3 to Qore nodes conversion.
        \param stmt a reference for sqlite3 SQL statement. It has to be
                    prepared and fetched already.
        \param index index of column to retrieve.
        \retval AbstractQoreNode* Qore node type by its sqlite afinity.
                    Possible nodes: QoreBigIntNode, QoreFloatNode, BinaryNode,
                                    QoreStringNode, and null().
        */
        AbstractQoreNode * columnValue(sqlite3_stmt * stmt, int index);

        /*! \brief Prepare a string (SQL statement) for variables binding.
        It performs some Qore DB bind syntax validity checks too.
        \param str a mutable Qore string with SQL statement
        \param args a list with bindable parameters
        \param xsink exceptions handler
        \retval bool true on success, false on error
        */
        bool parseForBind(QoreString & str, const QoreListNode *args, ExceptionSink *xsink);

        /*! \brief Bind arguments to the SQL statement.
        \param stmt a prepared sqlite3 statement.
        \param args a list with bindable parameters
        \param xsink exception handler
        \retval bool true on success, false on error
        */
        bool bindParameters(sqlite3_stmt * stmt, ExceptionSink *xsink);

        /*! \brief Internal implementation of select() DB API.
        \param ds a Datasource reference from Qore API.
        \param qstr a SQL statement from Qore API.
        \param args a list with bindable parameters from Qore API.
        \param binding flag if it should allow variable binding (true) or not (false)
        \param xsink exception handler.
        \retval AbstractQoreNode* QoreHashNode with coluns as keys, data as lists.
        */
        AbstractQoreNode * select_internal(
            Datasource *ds,
            const QoreString *qstr,
            const QoreListNode *args,
            bool binding,
            ExceptionSink *xsink);
};

#endif
