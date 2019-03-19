#!/usr/bin/env python3.4
"""
	Code by Noel O'Boyle copied/forked from 
	https://nextmovesoftware.com/blog/2018/04/09/textmining-pubmed-abstracts-with-leadmine/.
	Converts XML GZ PubMed archives to separate XML files in ZIP archives,
	for processing by NextMove LeadMine.
	Minor mods by Jeremy Yang.
"""
#
import os
import sys
import glob
import gzip
import zipfile
import multiprocessing as mp
import xml.etree.ElementTree as ET

class Details:
    def __init__(self, title, abstract, year, volume, journal, page):
        self.title = title
        self.abstract = abstract
        self.year = year
        self.volume = volume
        self.journal = journal
        self.page = page
    def __repr__(self):
        return "%s _%s_ *%s* _%s_ %s\n\nAbstract: %s" % (self.title, self.journal, self.year, self.volume, self.page, self.abstract)

def getelements(filename_or_file, tag):
    """Yield *tag* elements from *filename_or_file* xml incrementaly."""
    context = iter(ET.iterparse(filename_or_file, events=('start', 'end')))
    _, root = next(context) # get root element
    for event, elem in context:
        if event == 'end' and elem.tag == tag:
            yield elem
            root.clear() # free memory

def getText(node):
    return "" if node is None else node.text

def extract(medline):
    article = medline.find("Article")
    title = "".join(article.find("ArticleTitle").itertext())
    abstractNode = article.find("Abstract")
    abstract = ""
    if abstractNode is not None:
        abstract = []
        for abstractText in abstractNode.findall("AbstractText"):
            abstract.append("".join(abstractText.itertext()))
        abstract = " ".join(abstract)
    page = getText(article.find("Pagination/MedlinePgn"))
    journal = article.find("Journal")
    journalissue = journal.find("JournalIssue")
    volume = getText(journalissue.find("Volume"))
    year = getText(journalissue.find("PubDate/Year"))
    journaltitle = getText(journal.find("Title"))
    return Details(title, abstract, year, volume, journaltitle, page)

class PubMed:
    def __init__(self, fname):
        self.iter = self.getArticles(gzip.open(fname))

    def getArticles(self, mfile):
        for elem in getelements(mfile, "PubmedArticle"):
            medline = elem.find("MedlineCitation")
            pmidnode = medline.find("PMID")
            pmid = pmidnode.text
            version = pmidnode.get('Version')
            yield pmid, version, medline

    def getAll(self):
        for pmid, version, medline in self.iter:
            yield pmid, version, extract(medline)

    def getArticleDetails(self, mpmid):
        for pmid, _, medline in self.iter:
            if mpmid and mpmid != pmid: continue
            return extract(medline)

template = '<xml><article pmid="{0}" version="{1}" journal="{p.journal}" year="{p.year}" volume="{p.volume}" page="{p.page}"><title>{p.title}</title><abstract>{p.abstract}</abstract></article>'

def handleonefile(iotuple):
    ipath,opath = iotuple
    pm = PubMed(ipath)
    with zipfile.ZipFile(opath, mode="w", compression=zipfile.ZIP_DEFLATED) as out:
        for pmid, version, article in pm.getAll():
            text = (template.format(pmid, version, p=article))
            out.writestr("{}.{}.xml".format(pmid, version), text)

if __name__ == "__main__":
    poolsize = 4 # CPUs

    if len(sys.argv)<3:
      print("ERROR: Syntax: %s INDIR_XMLGZ OUTDIR_ZIP [NPROC]"%sys.argv[0], file=sys.stderr)
      sys.exit(1)

    idir = sys.argv[1]
    odir = sys.argv[2]
    if len(sys.argv)>3:
      poolsize = int(sys.argv[3])

    ipaths = glob.glob("%s/*.xml.gz"%idir)
    iotuples = []
    for ipath in ipaths:
      basename = os.path.basename(ipath).split(".")[0]
      opath = os.path.join(odir, basename+".zip")
      iotuples.append((ipath,opath))

    pool = mp.Pool(poolsize)
    i_file=0;
    for x in pool.imap_unordered(handleonefile, iotuples, 1):
      ipath,opath = iotuples[i_file]
      i_file+=1
      print("%d/%d. %s : %s (%.1f%%)"%(i_file,len(iotuples),ipath,opath,100*i_file/len(iotuples)), file=sys.stderr)

    print("N_FILES: %d"%i_file, file=sys.stderr)
