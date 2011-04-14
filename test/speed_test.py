#!/usr/bin/env python

import os, commands, random, unittest

class TestSpeed(unittest.TestCase):
  def setUp(self):
    pass

  def testSpeeds(self):
    print "testSpeeds:"

    # build fcompare
    os.system("make fcompare_test")

    min_i = 100000
    min_j = 100000
    min_k = 0

    i = min_i
    j = min_j
    k = min_k

    max_i = 10000000
    max_j = 10000000 
    max_k = 5

    fn1 = "data/names.1.100K.txt"
    fn2 = "data/names.2.100K.txt"

#    fn1 = "data/names.1.1M.txt"
#    fn2 = "data/names.2.1M.txt"

    min_time = 10000.0
    min_time_combination = None

    while (i <= max_i):
      j = min_j
      while (j <= max_j):
        k = min_k
        while (k <= max_k):
          print "%s %s %s" % (i, j, k) 
          res = commands.getoutput("time ./fcompare_test %s %s %s %s %s > /dev/null" % \
            (fn1, fn2, i, j, k))
          lines = res.split("\n")
          print lines[1]
          t  = float(lines[1].split()[1].split("m")[1].split("s")[0])
          if t < min_time:
            print "\n --> found new min_comb_time: %s (%s %s %s)\n" % \
              (t, i, j, k)
            min_time = t
            min_combination = "%s %s %s" % (i, j, k)
#          flush()          
          k += 1
        j *= 10
      i *= 10

    print "\n*******************************************************************\n"
    print "min_time: %s" % min_time
    print "min_combination: %s" % min_combination


if __name__ == '__main__':
  unittest.main() 
