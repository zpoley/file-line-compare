#!/usr/bin/env python

import os, commands, random, unittest

class TestMatchOutput(unittest.TestCase):
  def setUp(self):
    pass

  def testIdenticalLines(self):
    print "testIdenticalLines:"

    # build fcompare
    os.system("make")

    f1_name = "data/f1_test.txt"

    # quickly generate two files with a few differences
    f1 = open(f1_name, "w")

    for i in xrange(0, 100): # 100 lines
      for j in xrange(0, random.randint(0, 255)):
        f1.write("%s" % chr(random.randint(14, 127)))
      f1.write("%s\n" % str)

    f1.close()

    res = commands.getoutput("./fcompare %s %s" % (f1_name, f1_name))

    # assert comparing a file to itself produces no output
    self.assertEqual(res, "")

    os.remove(f1_name)

  def testDifferentLines(self):
    print "testDifferentLines:"

    res = commands.getoutput("./fcompare data/test_file_1.txt data/test_file_2.txt")

    diffs = [ "abc", "foo", "bar" ]

    for line in res.split("\n"):
      self.assertTrue(line in diffs)

if __name__ == '__main__':
  unittest.main() 
