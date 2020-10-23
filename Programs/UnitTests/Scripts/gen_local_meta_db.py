#!/usr/bin/python
# -*- coding: utf-8 -*-

import sqlite3 as lite
import sys
import os

con = None

try:
    os.remove('local_fake_meta.db')
    con = lite.connect('local_fake_meta.db')

    con.execute('CREATE TABLE IF NOT EXISTS packs(\"index\" INT PRIMARY KEY, name TEXT, dependency TEXT)')
    con.execute('CREATE TABLE IF NOT EXISTS files(path TEXT PRIMARY KEY, pack_index INTEGER NOT NULL)')

    con.execute('INSERT INTO files (path, pack_index) VALUES (\"some/not/existing/path\", 0)')
    con.execute('INSERT INTO files (path, pack_index) VALUES (\"some/not/existing/path2\", 1)')

    con.execute('INSERT INTO packs (\"index\", name, dependency) VALUES (0, "fake_pack_00", "")')
    con.execute('INSERT INTO packs (\"index\", name, dependency) VALUES (1, "fake_pack_01", "0")')
    con.commit()

except lite.Error, e:

    print "Error %s:" % e.args[0]
    sys.exit(1)

finally:
    if con:
        con.close()