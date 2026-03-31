# NextMove-based Tools


Convenience and integration tools build with the NextMove API, a commercial product,
including a web app utility, and command line app. Refer to
[NextMove](https://nextmovesoftware.com "NextMove website")
documentation.

## Dependencies

* [NextMove](https://nextmovesoftware.com "NextMove website") LeadMine


```
java -jar unm_biocomp_nextmove/target/unm_biocomp_nextmove-0.0.5-SNAPSHOT-jar-with-dependencies.jar 
usage: LeadMine_Utils [-config <CFILE>] [-h] -i <IFILE> [-idcol <IDCOL>]
       [-lbd <LBD>] [-max_corr_dist <MAX_CORR_DIST>] [-min_corr_entity_len
       <MIN_CE_LEN>] [-min_entity_len <MIN_E_LEN>] [-o <OFILE>]
       [-spellcorrect] [-textcol <TEXTCOL>] [-unquote] [-v]
LeadMine_Utils: Utility CLI powered by NextMove LeadMine (v4.2) chemical
and biomedical entity recognition
 -config <CFILE>                     Input configuration file
 -h,--help                           Show this help.
 -i <IFILE>                          Input file
 -idcol <IDCOL>                      # of ID input column
 -lbd <LBD>                          LeadMine look-behind depth
 -max_corr_dist <MAX_CORR_DIST>      LeadMine Max correction (Levenshtein)
                                     distance
 -min_corr_entity_len <MIN_CE_LEN>   LeadMine Min corrected entity length
 -min_entity_len <MIN_E_LEN>         LeadMine Min entity length
 -o <OFILE>                          Output file
 -spellcorrect                       LeadMine spelling correction
 -textcol <TEXTCOL>                  # of text/document input column
 -unquote                            unquote quoted column
 -v,--verbose                        Verbose.
Missing required option: i
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
mvn install:install-file -Dfile=/usr/local/leadmine-4.2/bin/leadmine.jar -DgroupId=nextmove -DartifactId=leadmine -Dversion=4.2 -Dpackaging=jar -DlocalRepositoryPath=/var/www/html/.m2/
```

Developed at the [UNM](http://www.unm.edu) Translational Informatics Division.

