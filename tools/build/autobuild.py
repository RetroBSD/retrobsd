#!/usr/bin/python
#
# Autobuild script for RetroBSD
#
import sys, string, os, subprocess, shutil, time, datetime, MySQLdb

BSD     = "/website/retrobsd/build/retrobsd-sources"
ARCHIVE = "/website/retrobsd/build/master"
REPO    = "https://github.com/RetroBSD/retrobsd.git"

#
# Get current date
#
today = datetime.date.today()
DATE = "%04d-%02d-%02d" % (today.year, today.month, today.day)
print "--- Started:", time.ctime()

#
# (1) If BSD directory exists: use 'git pull' to update
#     otherwise: check it out
#
if os.path.exists(BSD):
    print "--- Update the existing source tree"
    os.system("git -C "+BSD+" pull > /dev/null")
    fresh_sources = False
else:
    print "--- Checkout a fresh snapshot of sources"
    os.system("git clone "+REPO+" "+BSD)
    fresh_sources = True

#
# (2) Get the revision number REV
#
rev = subprocess.check_output(string.split("git -C "+BSD+" rev-list HEAD --count"))
rev = int(rev)
print "--- Latest revision:", rev

rid = subprocess.check_output(string.split("git -C "+BSD+" rev-parse --short HEAD")).strip()
print "--- Commit ID:", rid

#
# (3) If the REV already exists in the ARCHIVE: all done.
#     Otherwise proceed to (4).
#
if os.path.exists(ARCHIVE + "/" + str(rev)):
    print "--- Build for revision "+str(rev)+" already available"
    print "--- Finished:", time.ctime()
    sys.exit (0)

#
# (4) If the source tree is not fresh from (1), delete it
#     and checkout a new one.
#
if not fresh_sources:
    print "--- Checkout a fresh snapshot of sources"
    shutil.rmtree(BSD)
    os.system("git clone "+REPO+" "+BSD)

#
# (5) Pack the source tree into the zip archive.
#
print "--- Pack sources into sources-"+DATE+".zip"
os.mkdir(ARCHIVE + "/" + str(rev))
os.system("cd "+BSD+"/.. && zip -rq "+ARCHIVE+"/"+str(rev)+"/sources-"+DATE+".zip retrobsd-sources -x .git \\*/.gitignore")

#
# (6) Build everything in BSD/tools/build directory.
#
print "--- Build everything"
os.system("make -C "+BSD+"/tools/build all")

#
# (7) Move log and zip files into ARCHIVE.
#
print "--- Move log and zip files"
os.system("mv "+BSD+"/tools/build/*.log "+ARCHIVE+"/"+str(rev))
os.system("mv "+BSD+"/tools/build/*.zip "+ARCHIVE+"/"+str(rev))

#
# (8) Query mysql db for the total count of downloads
#     for the previous revision.
#
# (9) If no downloads, delete the previous revision.
#
mysql = MySQLdb.connect (user = "autobuild",
                           db = "autobuild",
                       passwd = "***")
for r in range(rev-1, 0, -1):
    # Skip non-existing revisions
    dir = ARCHIVE + "/" + str(r)
    if not os.path.exists(dir):
        continue

    # Get download count for this revision
    cur = mysql.cursor()
    cur.execute ("""
        SELECT count(*)
          FROM downloads
         WHERE branch = 'master'
           AND build = %d
        """ % r)
    count = cur.fetchall()[0][0]
    if count > 0:
        break

    print "--- Delete unused revision", r
    shutil.rmtree(dir)

print "--- Finished:", time.ctime()
sys.exit (0)
