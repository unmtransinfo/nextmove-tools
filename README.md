# NextMove-based Tools


Convenience and integration tools build with the NextMove API, a commercial product,
including a web app utility, and command line app. Refer to
[NextMove](http://nextmovesoftware.com "NextMove website")
documentation.

## Dependencies

* [NextMove](http://nextmovesoftware.com "NextMove website") LeadMine


```
mvn exec:java -Dexec.mainClass="edu.unm.health.biocomp.nextmove.leadmine_utils"

leadmine_utils - NextMove LeadMine chemical entity recognition
usage: leadmine_utils [options]
  required:
    -i IFILE ........................... input TSV file
  LeadMine options:
    -config CFILE ...................... input configuration file
    -spellcorrect ...................... spelling correction
    -max_correction_distance ........... max correction (Levenshtein) distance
    -min_corrected_entity_length LEN ... 
    -min_entity_length LEN ............. 
    -lbd LBD ........................... look-behind depth
  Misc options:
    -o OFILE ........................... output TSV file [stdout]
    -textcol COL ....................... # of text/document input column
    -idcol COL ......................... # of ID input column
    -unquote ........................... unquote quoted column
    -v ................................. verbose
    -vv ................................ very verbose
    -vvv ............................... very very verbose
    -h ................................. this help

```

## Java compilation

```
mvn clean install
```

## Maven local repo for LeadMine JAR

Not the only or maybe best way, but one way to configure Maven for a 
local dependency is via local HTTP Maven repository. See pom.xml, containing
specification:

```
<repositories>
  <repository>
    <id>local-repo</id>
    <url>http://localhost/.m2</url>
  </repository>
</repositories>
```

Libs may be installed into local repo thus:

```
mvn install:install-file -Dfile=leadmine-3.14.1.jar -DgroupId=nextmove -DartifactId=leadmine -Dversion=3.14.1 -Dpackaging=jar -DlocalRepositoryPath=/home/www/htdocs/.m2/
```

Developed at the [UNM](http://www.unm.edu) Translational Informatics Division.

