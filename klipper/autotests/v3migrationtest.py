#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import sqlite3
import subprocess
import sys
import tempfile
import time
import unittest
from typing import Final

import gi

gi.require_version('GdkPixbuf', '2.0')
from gi.repository import GdkPixbuf

CMAKE_RUNTIME_OUTPUT_DIRECTORY: Final = os.getenv("CMAKE_RUNTIME_OUTPUT_DIRECTORY", os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, os.pardir, "build", "bin"))


class V3MigrationTest(unittest.TestCase):
    """
    history2.lst -> history3.sqlite
    """

    v2_to_v3_bin: str
    history2_text_file: str
    history2_image_file: str
    time_now: int = int(time.time())

    @classmethod
    def setUpClass(cls) -> None:
        cls.v2_to_v3_bin = os.path.join(CMAKE_RUNTIME_OUTPUT_DIRECTORY, "plasma6.3-update-clipboard-database-2-to-3")
        cls.history2_text_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "onetextentry.lst")
        cls.history2_image_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data", "bug465225.lst")
        assert os.path.exists(cls.v2_to_v3_bin)
        assert os.path.exists(cls.history2_text_file)
        assert os.path.exists(cls.history2_image_file)

    @classmethod
    def tearDownClass(cls) -> None:
        pass

    def test_convert_v2_to_v3_text(self) -> None:

        with tempfile.TemporaryDirectory() as temp_dir:
            target_db = os.path.join(temp_dir, "history3.sqlite")
            subprocess.check_call([self.v2_to_v3_bin, "--input", self.history2_text_file, "--output", target_db], stdout=sys.stdout, stderr=sys.stderr)
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "data")))
            self.assertTrue(os.path.exists(target_db))
            for uuid, result in (("230aa750d982a8e1a7e8f0b7ccc4e4b1b87bf593", "Fushan Wen"), ("e2ab8561c5a8f9967e62486c44211c63bcf7d002", "clipboard")):
                self.assertTrue(os.path.exists(os.path.join(temp_dir, "data", uuid)))
                self.assertTrue(os.path.exists(os.path.join(temp_dir, "data", uuid, uuid)))
                with open(os.path.join(temp_dir, "data", uuid, uuid), encoding="utf-8") as fh:
                    self.assertEqual(fh.readline(), result)

            con = sqlite3.connect(target_db)
            cur = con.cursor()
            res_cur = cur.execute("SELECT uuid,added_time,mimetypes,text FROM main ORDER BY added_time DESC")

            res = res_cur.fetchone()
            self.assertEqual(res[0], "230aa750d982a8e1a7e8f0b7ccc4e4b1b87bf593")
            self.assertGreaterEqual(res[1], self.time_now)
            self.assertEqual(res[2], "text/plain")
            self.assertEqual(res[3], "Fushan Wen")

            # main
            res = res_cur.fetchone()
            self.assertEqual(res[0], "e2ab8561c5a8f9967e62486c44211c63bcf7d002")
            self.assertGreaterEqual(res[1], self.time_now - 1)
            self.assertEqual(res[2], "text/plain")
            self.assertEqual(res[3], "clipboard")

            # aux
            self.assertEqual(1, len(cur.execute("SELECT uuid,mimetype,data_uuid FROM aux WHERE uuid='230aa750d982a8e1a7e8f0b7ccc4e4b1b87bf593' AND mimetype='text/plain' AND data_uuid='230aa750d982a8e1a7e8f0b7ccc4e4b1b87bf593'").fetchall()))
            self.assertEqual(1, len(cur.execute("SELECT uuid,mimetype,data_uuid FROM aux WHERE uuid='e2ab8561c5a8f9967e62486c44211c63bcf7d002' AND mimetype='text/plain' AND data_uuid='e2ab8561c5a8f9967e62486c44211c63bcf7d002'").fetchall()))

    def test_convert_v2_to_v3_image(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            target_db = os.path.join(temp_dir, "history3.sqlite")
            subprocess.check_call([self.v2_to_v3_bin, "--input", self.history2_image_file, "--output", target_db], stdout=sys.stdout, stderr=sys.stderr)
            self.assertTrue(os.path.exists(target_db))
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "data")))
            uuid = "8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc"
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "data", uuid)))
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "data", uuid, uuid)))
            pixbuf = GdkPixbuf.Pixbuf.new_from_file(os.path.join(temp_dir, "data", uuid, uuid))
            self.assertIsNotNone(pixbuf)
            self.assertEqual(pixbuf.get_width(), 1610)
            self.assertEqual(pixbuf.get_height(), 1329)

            con = sqlite3.connect(target_db)
            cur = con.cursor()
            res = cur.execute("SELECT uuid,added_time,mimetypes,text FROM main ORDER BY last_used_time DESC, added_time DESC").fetchall()
            self.assertEqual(len(res), 1)

            # main
            self.assertEqual(res[0][0], "8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc")
            self.assertGreaterEqual(res[0][1], self.time_now)
            self.assertEqual(res[0][2], "image/png")
            self.assertIsNone(res[0][3])

            # aux
            self.assertEqual(1, len(cur.execute("SELECT uuid,mimetype,data_uuid FROM aux WHERE uuid='8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc' AND mimetype='image/png' AND data_uuid='8f9353dabfdcf9aca5a901cd2c4ae6717cac5adc'").fetchall()))


if __name__ == '__main__':
    unittest.main()
