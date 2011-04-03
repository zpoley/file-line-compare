#!/usr/bin/env python

import unittest

class TestMatchOutput(unittest.TestCase):
  def setUp(self):
    pass

  def testMatchOutput(self):
    print "testMatchOutput:"

'''
//  char *filename1 = "data/test_file_1.txt";
//  char *filename2 = "data/test_file_2.txt";

//  char *filename1 = "data/names.1.100.txt";
//  char *filename2 = "data/names.2.100.txt";

//  char *filename1 = "data/names.3.100.txt";
//  char *filename2 = "data/names.4.100.txt";

//  char *filename1 = "data/names.5.100K.txt";
//  char *filename2 = "data/names.6.100K.txt";
'''



if __name__ == '__main__':
  unittest.main() 
