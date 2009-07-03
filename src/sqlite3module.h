/*
  sqlite3module.cc

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

#ifndef SQLITE3MODULE_H
#define SQLITE3MODULE_H

QoreStringNode *qore_sqlite3_module_init();
void qore_sqlite3_module_ns_init(QoreNamespace *rns, QoreNamespace *qns);
void qore_sqlite3_module_delete(void);

#endif
