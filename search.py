"""
Search engine
"""

import os,sys
import argparse
import re

DB_DIR 		= "./server/database/"
RESULT_DIR 	= "./result.html"

def generate_html(ranked_list_pg, keyword):
	fout = open(RESULT_DIR, 'w+')
	for pg in ranked_list_pg:
		print >>fout,'<link href="format.css" rel="stylesheet"/>'
		content = pg.get_content()
		title_search = re.search('<title>(.*)</title>', content, re.IGNORECASE)
		print >>fout,'<a href="%s">' % pg.get_url()
		if title_search:
			title = title_search.group(1)
			print >>fout,title 
		else:
			print >>fout,'Untitled Document'
		print >>fout,"</a>"

		print >>fout,'<div class="url">'
		print >>fout,pg.get_url()
		print >>fout,'</div>'
		print >>fout,"</br>"
	
class Page(object):
	def __init__(self, url, content):
		self.url = url
		self.content = content
		self.ri = 0

	def get_url(self):
		return self.url

	def get_content(self):
		return self.content

	def get_ri(self):
		return self.ri

	def set_ri(self, ri):
		self.ri = ri

	# algorithm
	def calc_ri(self, kw):
		kw_set = kw.split(" ")
		for kw in kw_set:
			count = self.content.count(kw)
			self.ri += count 
			count = self.url.count(kw)
			self.ri += (count*500)
##
# Main program
##

parser = argparse.ArgumentParser(description=\
	'Simple Search Engine for NUS')
parser.add_argument('--keyword', dest='kw',\
	help='KEY WORD to start searching')
args = parser.parse_args()

keyword = args.kw
if keyword == None:
	print 'please enter a NUS-related keyword'
	sys.exit(-1)

list_pg = []
list_ri = []
files = os.listdir(DB_DIR)

if not files:
	print "Empty database."
	sys.exit(-1)

for f in files:
	f_dir = DB_DIR + f
	fout = open(f_dir, 'r')
	content = fout.read()
	url = f.replace('*', '/')
	pg = Page(url, content)
	pg.calc_ri(keyword)
	list_pg.append(pg)
	list_ri.append(pg.get_ri())

ranked_list_pg = []
list_ri.sort(reverse=True)
for ri in list_ri:
	for pg in list_pg:
		if ri == pg.get_ri():
			ranked_list_pg.append(pg)
if len(ranked_list_pg) <= 10:
	generate_html(ranked_list_pg, keyword)
else:
	generate_html(ranked_list_pg[0:9], keyword)
