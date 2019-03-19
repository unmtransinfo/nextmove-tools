package edu.unm.health.biocomp.nextmove;

import java.io.*;
import java.util.zip.GZIPInputStream;
import java.util.*;
import java.util.regex.*;

import edu.unm.health.biocomp.util.*;

import com.nextmovesoftware.leadmine.*; // LeadMine, Entity, EntityCollector, CsvOutput
import com.nextmovesoftware.leadmine.TsvOutput.*; // 

/**	NextMove LeadMine chemical entity recognition, name to molecule.
	Chemical mention entity types:
	"M" indicates a systematically named molecule.
	"E" indicates an element or mono-atomic name.
	"D" indicates a dictionary molecule name.
	"C" indicates a Chemical Abstracts Service number.
	"R" indicates a pharmaceutical registry number.
	"A" indicates an atomic (single atom) substituent prefix.
	"P" indicates a polyatomic substituent prefix.
	"Y" indicates a polymer.
	"F" indicates a formula.
	"G" indicates a generic term.
*/
public class leadmine_utils
{
  /////////////////////////////////////////////////////////////////////////////
  private static void Help(String msg)
  {
    System.err.println(msg+"\n"
      +"leadmine_utils - NextMove LeadMine chemical entity recognition\n"
      +"usage: leadmine_utils [options]\n"
      +"  required:\n"
      +"    -i IFILE ........................... input TSV file (.tsv or .tsv.gz)\n"
      +"  LeadMine options:\n"
      +"    -config CFILE ...................... input configuration file\n"
      +"    -spellcorrect ...................... spelling correction\n"
      +"    -max_correction_distance ........... max correction (Levenshtein) distance\n"
      +"    -min_corrected_entity_length LEN ... \n"
      +"    -min_entity_length LEN ............. \n"
      +"    -lbd LBD ........................... look-behind depth\n"
      +"  Misc options:\n"
      +"    -o OFILE ........................... output TSV file [stdout]\n"
      +"    -textcol COL ....................... # of text/document input column\n"
      +"    -idcol COL ......................... # of ID input column\n"
      +"    -unquote ........................... unquote quoted column\n"
      +"    -v ................................. verbose\n"
      +"    -vv ................................ very verbose\n"
      +"    -vvv ............................... very very verbose\n"
      +"    -h ................................. this help\n");
    System.exit(1);
  }
  private static int verbose=0;
  private static String ifile=null;
  private static String ofile=null;
  private static String cfile=null;
  private static Integer textcol=1;
  private static Integer idcol=null;
  private static boolean unquote=false;
  private static boolean spellcorrect=false;
  private static int max_correction_distance=1;
  private static int min_corrected_entity_length=7;
  private static int min_entity_length=3;

  /////////////////////////////////////////////////////////////////////////////
  private static void parseCommand(String args[])
  {
    for (int i=0;i<args.length;++i)
    {
      if (args[i].equals("-i")) ifile=args[++i];
      else if (args[i].equals("-o")) ofile=args[++i];
      else if (args[i].equals("-config")) cfile=args[++i];
      else if (args[i].equals("-textcol")) textcol=Integer.parseInt(args[++i]);
      else if (args[i].equals("-idcol")) idcol=Integer.parseInt(args[++i]);
      else if (args[i].equals("-max_correction_distance")) max_correction_distance=Integer.parseInt(args[++i]);
      else if (args[i].equals("-min_corrected_entity_length")) min_corrected_entity_length=Integer.parseInt(args[++i]);
      else if (args[i].equals("-min_entity_length")) min_entity_length=Integer.parseInt(args[++i]);
      else if (args[i].equals("-unquote")) unquote=true;
      else if (args[i].equals("-spellcorrect")) spellcorrect=true;
      else if (args[i].equals("-v")) verbose=1;
      else if (args[i].equals("-vv")) verbose=2;
      else if (args[i].equals("-vvv")) verbose=3;
      else Help("Unknown option: "+args[i]);
    }
  }
  /////////////////////////////////////////////////////////////////////////////
  /**   Main for utility application.
  */
  public static void main(String[] args)
    throws IOException
  {
    parseCommand(args);
    if (ifile==null) Help("Input file required.");
    String ifilename = ifile.replaceFirst("^.*/", "");

    BufferedReader buff = null;
    if (ifile.matches("^.*\\.gz$")) {
      InputStream gzstrm = new GZIPInputStream(new FileInputStream(ifile));
      buff = new BufferedReader(new InputStreamReader(gzstrm, "UTF-8"));
    }
    else {
      buff = new BufferedReader(new FileReader(ifile));
    }

    OutputStream fos = (ofile!=null) ?
      (new FileOutputStream(new File(ofile)))
      : ((OutputStream)System.out);

    BufferedWriter fob = new BufferedWriter(new OutputStreamWriter(fos));

    // Configuration:
    LeadMineConfig leadMineConfig = (cfile==null) ?
      (new LeadMineConfig()) : (new LeadMineConfig(new File(cfile)));

    SpellingCorrectorConfig spellingCorrectorConfig = leadMineConfig.getSpellingCorrectorConfig();

    List<CfxDictionary> leadMineDictionaries = leadMineConfig.getDictionaries();
    if (spellcorrect) {
      for (CfxDictionary cfxd: leadMineDictionaries) { // Modify per dictionary:
        cfxd.setCaseSensitive(false);
        cfxd.setUseSpellingCorrection(spellcorrect);
        cfxd.setMaxCorrectionDistance(max_correction_distance);
        cfxd.setMinimumEntityLength(min_entity_length);
        cfxd.setMinimumCorrectedEntityLength(min_corrected_entity_length);
      }
    }

    // Instantiate LeadMine:
    LeadMine leadMine = new LeadMine(leadMineConfig);

    // Check configuration:
    System.err.println("LeadMine version: "+leadMine.getVersion());
    if (verbose>1) {
      System.err.println("LeadMine lookBehindDepth: "+spellingCorrectorConfig.getExhaustiveLookBehindDepth());
      Set<String> entityTypes = leadMineConfig.getExpectedEntityTypes();
      for (String entityType: entityTypes) {
        System.err.println("LeadMine entityType: "+entityType);
      }
      for (CfxDictionary cfxd: leadMineDictionaries) {
        System.err.print("LeadMine dictionary: "+cfxd.getSource());
        System.err.print(" ; case_sensitive="+cfxd.isCaseSensitive());
        System.err.print(" ; spellcorrect="+cfxd.useSpellingCorrection());
        System.err.print(" ; max_correction_distance="+cfxd.getMaxCorrectionDistance());
        System.err.print(" ; min_entity_length="+cfxd.getMinimumEntityLength());
        System.err.println(" ; min_corrected_entity_length="+cfxd.getMinimumCorrectedEntityLength());
      }
    }
    boolean use_spellcorrect = spellcorrect;
    for (CfxDictionary cfxd: leadMineDictionaries) {
      use_spellcorrect |= cfxd.useSpellingCorrection();
    }

    TsvOutputBuilder tsvOutputBuilder = new TsvOutput.TsvOutputBuilder();
    tsvOutputBuilder = tsvOutputBuilder
	.withResolvers(leadMineConfig.getNameResolvers())
	.withCol(TsvOutput.TsvOutputColumn.DocName) //Input rownum.
	.withCol(TsvOutput.TsvOutputColumn.SectionType)
	.withCol(TsvOutput.TsvOutputColumn.OriginalText)
	.withCol(TsvOutput.TsvOutputColumn.EntityType)
	.withCol(TsvOutput.TsvOutputColumn.BegIndex)
	.withCol(TsvOutput.TsvOutputColumn.EntityText);
    if (spellcorrect || use_spellcorrect) {
      tsvOutputBuilder = tsvOutputBuilder
	.withCol(TsvOutput.TsvOutputColumn.PossiblyCorrectedText)
	.withCol(TsvOutput.TsvOutputColumn.CorrectionDistance);
    }
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.ResolvedForm);

    TsvOutput tsvOut = tsvOutputBuilder.build();

    fob.write(tsvOut.getTsvHeader()+"\n");
    fob.flush();

    java.util.Date t_0 = new java.util.Date();
    int i_input=0;
    int n_ner_none=0;
    int n_ner=0;
    String line=null;
    line=buff.readLine(); //header
    String[] tags = line.split("\t");

    while ((line=buff.readLine())!=null)
    {
      ++i_input;
      String[] vals = line.split("\t");
      String doctext = null;
      try { doctext = vals[textcol-1]; }
      catch (Exception e) {
        System.err.println("ERROR: badly formed line: ["+i_input+"] \""+line+"\" ; Exception ("+e.toString()+")");
        continue;
      }
      String idtext = (idcol==null)?(""+i_input):(vals[idcol-1]);
      if (unquote) {
        doctext = doctext.replaceFirst("^\"(.*)\"$","$1");
        doctext = doctext.replaceAll("\"\"","\"");
        doctext = doctext.replaceAll("\\\"","\"");
      }
      EntityCollector collector = leadMine.findEntities(Normalizer.normalizeTxtToStr(doctext));

      tsvOut.outputTsv(collector, fos, idtext); //1-line/entity, "" for none
      List<Entity> entities = collector.getEntities();
      int n_ner_this=entities.size();

      n_ner+=n_ner_this;
      if (n_ner_this==0) {
        if (verbose>2)
          System.err.println("NOTE: NER fails: ["+i_input+"] \""+doctext+"\"");
        ++n_ner_none;
      }
    }
    buff.close();
    fob.close();
    fos.close();
    System.err.println("Input docs: "+i_input);
    System.err.println("Output NER: "+n_ner);
    System.err.println("NER fails: "+n_ner_none);
    System.err.println("total elapsed time: "+time_utils.TimeDeltaStr(t_0,new java.util.Date()));
  }
}
