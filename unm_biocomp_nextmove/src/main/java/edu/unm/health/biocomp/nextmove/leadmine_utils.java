package edu.unm.health.biocomp.nextmove;

import java.io.*;
import java.util.zip.GZIPInputStream;
import java.util.*;
import java.util.regex.*;

import org.apache.commons.cli.*; // CommandLine, CommandLineParser, HelpFormatter, OptionBuilder, Options, ParseException, PosixParser
import org.apache.commons.cli.Option.*; // Builder

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
  private static String APPNAME = "LeadMine_Utils";
  private static String ifile=null;
  private static String ofile=null;
  private static String cfile=null;
  private static Integer textcol=1;
  private static Integer idcol=null;
  private static Integer max_corr_dist=1;
  private static Integer min_corr_entity_len=7;
  private static Integer min_entity_len=3;
  private static Integer verbose=0;
  private static Boolean unquote=false;
  private static Boolean spellcorrect=false;

  /////////////////////////////////////////////////////////////////////////////
  /**   Main for utility application.
  */
  public static void main(String[] args) throws Exception
  {
    String HELPHEADER = APPNAME+": Utility CLI powered by NextMove LeadMine (v"+(new LeadMine()).getVersion()+") chemical and biomedical entity recognition";
    Options opts = new Options();
    opts.addOption(Option.builder("i").required().hasArg().argName("IFILE").desc("Input file").build());
    opts.addOption(Option.builder("o").hasArg().argName("OFILE").desc("Output file").build());
    opts.addOption(Option.builder("config").hasArg().argName("CFILE").desc("Input configuration file").build());
    opts.addOption(Option.builder("textcol").hasArg().argName("TEXTCOL").desc("# of text/document input column").build());
    opts.addOption(Option.builder("idcol").hasArg().argName("IDCOL").desc("# of ID input column").build());
    opts.addOption(Option.builder("unquote").desc("unquote quoted column").build());
    opts.addOption(Option.builder("spellcorrect").desc("LeadMine spelling correction").build());
    opts.addOption(Option.builder("max_corr_dist").hasArg().argName("MAX_CORR_DIST").desc("LeadMine Max correction (Levenshtein) distance").build());
    opts.addOption(Option.builder("min_entity_len").hasArg().argName("MIN_E_LEN").desc("LeadMine Min entity length").build());
    opts.addOption(Option.builder("min_corr_entity_len").hasArg().argName("MIN_CE_LEN").desc("LeadMine Min corrected entity length").build());
    opts.addOption(Option.builder("lbd").hasArg().argName("LBD").desc("LeadMine look-behind depth").build());
    opts.addOption("v", "verbose", false, "Verbose.");
    opts.addOption("h", "help", false, "Show this help.");
    HelpFormatter helper = new HelpFormatter();
    CommandLineParser clip = new PosixParser();
    CommandLine clic = null;
    try {
      clic = clip.parse(opts, args);
    } catch (ParseException e) {
      helper.printHelp(APPNAME, HELPHEADER, opts, e.getMessage(), true);
      System.exit(0);
    }
    ifile = clic.getOptionValue("i");
    if (clic.hasOption("o")) ofile = clic.getOptionValue("o");
    if (clic.hasOption("config")) cfile = clic.getOptionValue("config");
    if (clic.hasOption("textcol")) { textcol = Integer.parseInt(clic.getOptionValue("textcol")); }
    if (clic.hasOption("idcol")) { idcol = Integer.parseInt(clic.getOptionValue("idcol")); }
    if (clic.hasOption("max_corr_dist")) { max_corr_dist = Integer.parseInt(clic.getOptionValue("max_corr_dist")); }
    if (clic.hasOption("min_entity_len")) { min_entity_len = Integer.parseInt(clic.getOptionValue("min_entity_len")); }
    if (clic.hasOption("min_corr_entity_len")) { min_corr_entity_len = Integer.parseInt(clic.getOptionValue("min_corr_entity_len")); }
    if (clic.hasOption("unquote")) { unquote = true; }
    if (clic.hasOption("spellcorrect")) { spellcorrect = true; }
    if (clic.hasOption("v")) { verbose = 1; }
    if (clic.hasOption("h")) {
      helper.printHelp(APPNAME, HELPHEADER, opts, "", true);
      System.exit(0);
    }

    String ifilename = ifile.replaceFirst("^.*/", "");

    BufferedReader buff = null;
    if (ifile.matches("^.*\\.gz$")) {
      InputStream gzstrm = new GZIPInputStream(new FileInputStream(ifile));
      buff = new BufferedReader(new InputStreamReader(gzstrm, "UTF-8"));
    }
    else {
      buff = new BufferedReader(new FileReader(ifile));
    }

    OutputStream fos = (ofile!=null) ? (new FileOutputStream(new File(ofile))) : ((OutputStream)System.out);
    BufferedWriter fob = new BufferedWriter(new OutputStreamWriter(fos));

    // Configuration:
    LeadMineConfig leadMineConfig = (cfile==null) ? (new LeadMineConfig()) : (new LeadMineConfig(new File(cfile)));
    SpellingCorrectorConfig spellingCorrectorConfig = leadMineConfig.getSpellingCorrectorConfig();
    LeadMine leadMine = new LeadMine(leadMineConfig); // Instantiate LeadMine

    List<CfxDictionary> leadMineDictionaries = leadMineConfig.getDictionaries();
    if (spellcorrect) {
      for (CfxDictionary cfxd: leadMineDictionaries) { // Modify per dictionary:
        cfxd.setCaseSensitive(false);
        cfxd.setUseSpellingCorrection(spellcorrect);
        cfxd.setMaxCorrectionDistance(max_corr_dist);
        cfxd.setMinimumEntityLength(min_entity_len);
        cfxd.setMinimumCorrectedEntityLength(min_corr_entity_len);
      }
    }

    System.err.println(HELPHEADER);

    // Check configuration:
    if (verbose>1) {
      System.err.println(APPNAME+": LeadMine lookBehindDepth: "+spellingCorrectorConfig.getExhaustiveLookBehindDepth());
      Set<String> entityTypes = leadMineConfig.getExpectedEntityTypes();
      for (String entityType: entityTypes) {
        System.err.println(APPNAME+": LeadMine entityType: "+entityType);
      }
      for (CfxDictionary cfxd: leadMineDictionaries) {
        System.err.print(APPNAME+": LeadMine dictionary: "+cfxd.getSource());
        System.err.print(" ; case_sensitive="+cfxd.isCaseSensitive());
        System.err.print(" ; spellcorrect="+cfxd.useSpellingCorrection());
        System.err.print(" ; max_correction_distance="+cfxd.getMaxCorrectionDistance());
        System.err.print(" ; min_entity_length="+cfxd.getMinimumEntityLength());
        System.err.println(" ; min_corrected_entity_length="+cfxd.getMinimumCorrectedEntityLength());
      }
    }
    Boolean use_spellcorrect = spellcorrect;
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
    int n_empty=0;
    int n_err=0;
    String line=null;
    line=buff.readLine(); //header
    String[] tags = line.split("\t");

    while ((line=buff.readLine())!=null)
    {
      ++i_input;
      String[] vals = line.split("\t", tags.length); //limit needed or trailing empty string discarded.
      String doctext = null;
      if (vals.length < textcol) {
        System.err.println("ERROR: insufficient columns ("+vals.length+"<"+textcol+"): ["+i_input+"] \""+line+"\"");
        ++n_err;
        continue;
      }
      try { doctext = vals[textcol-1]; }
      catch (Exception e) {
        System.err.println("ERROR: badly formed line: ["+i_input+"] \""+line+"\" ; Exception ("+e.toString()+")");
        ++n_err;
        continue;
      }
      if (doctext.isEmpty()) {
        ++n_empty;
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
      int n_ner_this = entities.size();

      n_ner+=n_ner_this;
      if (n_ner_this==0) {
        if (verbose>2)
          System.err.println("NOTE: NER_none: ["+i_input+"] \""+doctext+"\"");
        ++n_ner_none;
      }
    }
    buff.close();
    fob.close();
    fos.close();
    System.err.println(APPNAME+": Input docs: "+i_input);
    System.err.println(APPNAME+": Empty docs: "+n_empty);
    System.err.println(APPNAME+": Output NER: "+n_ner);
    System.err.println(APPNAME+": NER_none count: "+n_ner_none);
    System.err.println(APPNAME+": Errors: "+n_err);
    System.err.println(APPNAME+": total elapsed time: "+time_utils.TimeDeltaStr(t_0,new java.util.Date()));
  }
}
