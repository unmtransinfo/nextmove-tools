package edu.unm.health.biocomp.nextmove;

import java.io.*;
import java.lang.Math;
import java.net.*; //URLEncoder, InetAddress
import java.text.*;
import java.util.*;
import java.util.regex.*;
import java.util.zip.*;
import javax.servlet.*;
import javax.servlet.http.*;

import com.oreilly.servlet.MultipartRequest;
import com.oreilly.servlet.multipart.DefaultFileRenamePolicy;
import com.oreilly.servlet.*; //Base64Encoder, Base64Decoder

import com.nextmovesoftware.leadmine.*; // LeadMine, Entity, EntityCollector, TsvOutput
import com.nextmovesoftware.leadmine.TsvOutput.*; // TsvOutputBuilder

import edu.unm.health.biocomp.util.*;
import edu.unm.health.biocomp.util.http.*;

/**	Name to molecule conversion with NextMove LeadMine.
	@author Jeremy J Yang
*/
public class leadmine_servlet extends HttpServlet
{
  private static String SERVLETNAME=null;
  private static String CONTEXTPATH=null;
  private static String LOGDIR=null;
  private static String APPNAME=null;   // configured in web.xml
  private static String UPLOADDIR=null;   // configured in web.xml
  private static int N_MAX=100; // configured in web.xml
  private static ServletContext CONTEXT=null;
  //private static ServletConfig CONFIG=null;
  private static ResourceBundle rb=null;
  private static PrintWriter out=null;
  private static ArrayList<String> outputs=new ArrayList<String>();
  private static ArrayList<String> errors=new ArrayList<String>();
  private static HttpParams params=new HttpParams();
  private static String SERVERNAME=null;
  private static String REMOTEHOST=null;
  private static Calendar calendar=Calendar.getInstance();
  private static String datestr=null;
  private static File logfile=null;
  private static byte[] inbytes=null;
  private static ArrayList<String> texts=new ArrayList<String>();
  private static String color1="#EEEEEE";
  private static String PROXY_PREFIX=null;   // configured in web.xml

  /////////////////////////////////////////////////////////////////////////////
  public void doPost(HttpServletRequest request,HttpServletResponse response)
      throws IOException,ServletException
  {
    SERVERNAME=request.getServerName();
    if (SERVERNAME.equals("localhost")) SERVERNAME=InetAddress.getLocalHost().getHostAddress();
    REMOTEHOST=request.getRemoteHost(); // client
    rb=ResourceBundle.getBundle("LocalStrings",request.getLocale());

    MultipartRequest mrequest=null;
    if (request.getMethod().equalsIgnoreCase("POST"))
    {
      try {
        mrequest=new MultipartRequest(request,UPLOADDIR,10*1024*1024,"ISO-8859-1",new DefaultFileRenamePolicy());
      }
      catch (IOException lEx) {
        this.getServletContext().log("not a valid MultipartRequest",lEx);
      }
    }

    // main logic:
    ArrayList<String> cssincludes = new ArrayList<String>(Arrays.asList(PROXY_PREFIX+CONTEXTPATH+"/css/biocomp.css"));
    ArrayList<String> jsincludes = new ArrayList<String>(Arrays.asList(PROXY_PREFIX+CONTEXTPATH+"/js/biocomp.js", PROXY_PREFIX+CONTEXTPATH+"/js/ddtip.js"));

    boolean ok=initialize(request,mrequest);
    if (!ok)
    {
      response.setContentType("text/html");
      out=response.getWriter();
      out.print(HtmUtils.HeaderHtm(SERVLETNAME, jsincludes, cssincludes, JavaScript(), "", color1, request));
      out.print(HtmUtils.FooterHtm(errors,true));
    }
    else if (mrequest!=null)         //method=POST, normal operation
    {
      if (mrequest.getParameter("txt2mol").equalsIgnoreCase("TRUE"))
      {
        response.setContentType("text/html");
        out=response.getWriter();
        Name2Mol(mrequest, response);
        out.print(HtmUtils.HeaderHtm(SERVLETNAME, jsincludes, cssincludes, JavaScript(), "", color1, request));
        out.println(FormHtm(mrequest, response));
        out.println(HtmUtils.OutputHtm(outputs));
        out.print(HtmUtils.FooterHtm(errors, true));
      }
    }
    else
    {
      String help=request.getParameter("help"); // GET param
      String downloadtxt=request.getParameter("downloadtxt"); // POST param
      String downloadfile=request.getParameter("downloadfile"); // POST param
      if (help!=null)   // GET method, help=TRUE
      {
        response.setContentType("text/html");
        out=response.getWriter();
        out.print(HtmUtils.HeaderHtm(SERVLETNAME, jsincludes, cssincludes, JavaScript(), "", color1, request));
        out.println(HelpHtm());
        out.print(HtmUtils.FooterHtm(errors, true));
      }
      else if (downloadtxt!=null && downloadtxt.length()>0) // POST param
      {
        ServletOutputStream ostream=response.getOutputStream();
        HtmUtils.DownloadString(response, ostream, downloadtxt,
          request.getParameter("fname"));
      }
      else if (downloadfile!=null && downloadfile.length()>0) // POST param
      {
        ServletOutputStream ostream=response.getOutputStream();
        HtmUtils.DownloadFile(response, ostream, downloadfile,
          request.getParameter("fname"));
      }
      else      // GET method, initial invocation of servlet w/ no params
      {
        response.setContentType("text/html");
        out=response.getWriter();
        out.print(HtmUtils.HeaderHtm(SERVLETNAME, jsincludes, cssincludes, JavaScript(), "", color1, request));
        out.println(FormHtm(mrequest, response));
        out.println("<SCRIPT>go_init(window.document.mainform)</SCRIPT>");
        out.print(HtmUtils.FooterHtm(errors, true));
      }
    }
  }
  /////////////////////////////////////////////////////////////////////////////
  // /cdkdepict/depict/cow/png?smi=NCCc1cc(O)c(O)cc1
  // cow|bow, svg|png
  private boolean initialize(HttpServletRequest request, MultipartRequest mrequest)
      throws IOException, ServletException
  {
    SERVLETNAME=this.getServletName();
    params.clear();
    outputs.clear();
    errors.clear();

    String logo_htm="<TABLE CELLSPACING=5 CELLPADDING=5><TR><TD>";
    String imghtm=("<IMG BORDER=0 SRC=\""+PROXY_PREFIX+CONTEXTPATH+"/images/biocomp_logo_only.gif\">");
    String tiphtm=(APPNAME+" web app from UNM Translational Informatics.");
    String href=("http://datascience.unm.edu/");
    logo_htm+=(HtmUtils.HtmTipper(imghtm, tiphtm, href, 200, "white"));
    logo_htm+="</TD><TD>";
    imghtm=("<IMG BORDER=0 HEIGHT=90 SRC=\""+PROXY_PREFIX+CONTEXTPATH+"/images/nextmove.png\">");
    tiphtm=("LeadMine from NextMove.");
    href=("http://www.nextmovesoftware.com");
    logo_htm+=(HtmUtils.HtmTipper(imghtm, tiphtm, href, 200, "white"));
    logo_htm+="</TD></TR></TABLE>";
    errors.add(logo_htm);

    inbytes=new byte[1024];

    //Create webapp-specific log dir if necessary:
    File dout=new File(LOGDIR);
    if (!dout.exists())
    {
      boolean ok=dout.mkdir();
      System.err.println("LOGDIR creation "+(ok?"succeeded":"failed")+": "+LOGDIR);
      if (!ok)
      {
        errors.add("ERROR: could not create LOGDIR: "+LOGDIR);
        return false;
      }
    }

    String logpath=LOGDIR+"/"+SERVLETNAME+".log";
    logfile=new File(logpath);
    if (!logfile.exists())
    {
      logfile.createNewFile();
      logfile.setWritable(true, true);
      PrintWriter out_log=new PrintWriter(logfile);
      out_log.println("date\tip\tN");
      out_log.flush();
      out_log.close();
    }
    if (!logfile.canWrite())
    {
      errors.add("ERROR: Log file not writable.");
      return false;
    }
    BufferedReader buff=new BufferedReader(new FileReader(logfile));
    if (buff==null)
    {
      errors.add("ERROR: Cannot open log file.");
      return false;
    }

    int n_lines=0;
    String line=null;
    String startdate=null;
    while ((line=buff.readLine())!=null)
    {
      ++n_lines;
      String[] fields=Pattern.compile("\\t").split(line);
      if (n_lines==2) startdate=fields[0];
    }
    if (n_lines>2)
    {
      calendar.set(Integer.parseInt(startdate.substring(0, 4)),
               Integer.parseInt(startdate.substring(4, 6))-1,
               Integer.parseInt(startdate.substring(6, 8)),
               Integer.parseInt(startdate.substring(8, 10)),
               Integer.parseInt(startdate.substring(10, 12)),0);

      DateFormat df=DateFormat.getDateInstance(DateFormat.FULL, Locale.US);
      errors.add("since "+df.format(calendar.getTime())+", times used: "+(n_lines-1));
    }

    calendar.setTime(new Date());
    datestr=String.format("%04d%02d%02d%02d%02d",
      calendar.get(Calendar.YEAR),
      calendar.get(Calendar.MONTH)+1,
      calendar.get(Calendar.DAY_OF_MONTH),
      calendar.get(Calendar.HOUR_OF_DAY),
      calendar.get(Calendar.MINUTE));

    if (mrequest==null) return true;

    /// stuff for a run:

    for (Enumeration e=mrequest.getParameterNames(); e.hasMoreElements(); )
    {
      String key=(String)e.nextElement();
      if (mrequest.getParameter(key)!=null)
        params.setVal(key, mrequest.getParameter(key));
    }

    if (params.isChecked("verbose"))
    {
      errors.add("server: "+CONTEXT.getServerInfo()+" [API:"+CONTEXT.getMajorVersion()+"."+CONTEXT.getMinorVersion()+"]");
    }

    if (params.isChecked("txt2mol"))
    {
      File fin=mrequest.getFile("ifile");
      String intxt=params.getVal("intxt").replaceFirst("[\\s]+$", "");
      if (fin!=null)
      {
        FileInputStream fis=new FileInputStream(fin);
        int asize=inbytes.length;
        int size=0;
        int b;
        while ((b=fis.read())>=0)
        {
          if (size+1>asize)
          {
            asize*=2;
            byte[] tmp=new byte[asize];
            System.arraycopy(inbytes, 0, tmp, 0, size);
            inbytes=tmp;
          }
          inbytes[size]=(byte)b;
          ++size;
        }
        fin.delete();
        byte[] tmp=new byte[size];
        System.arraycopy(inbytes, 0, tmp, 0, size);
        inbytes=tmp;
        if (params.isChecked("file2txt"))
        {
          intxt=new String(inbytes, "utf-8");
          params.setVal("intxt", intxt);
        }
      }
      else if (intxt.length()>0)
      {
        inbytes=intxt.getBytes("utf-8");
      }
      if (inbytes==null)
      {
        errors.add("ERROR: no input texts");
        return false;
      }
      intxt=new String(inbytes, "utf-8");
      buff=new BufferedReader(new StringReader(intxt));
      texts.clear();
      while ((line=buff.readLine())!=null)
      {
        texts.add(line);
        if (texts.size()==N_MAX)
        {
          errors.add("N_MAX limit reached: "+N_MAX);
          break;
        }
      }
      if (params.isChecked("verbose"))
        errors.add("texts read:  "+texts.size());
    }
    else
    {
      errors.add("ERROR: Aaack!");	//should never happen
      return false;
    }
    return true;
  }
  /////////////////////////////////////////////////////////////////////////////
  private static String FormHtm(MultipartRequest mrequest, HttpServletResponse response)
      throws IOException, ServletException
  {

    String max_corr_dist_1=""; String max_corr_dist_2="";
    if (params.getVal("max_corr_dist").equals("1")) max_corr_dist_1="CHECKED";
    else if (params.getVal("max_corr_dist").equals("2")) max_corr_dist_2="CHECKED";
    else max_corr_dist_1="CHECKED";

    String min_ent_len_7=""; String min_ent_len_8=""; String min_ent_len_9=""; String min_ent_len_10="";
    if (params.getVal("min_ent_len").equals("7")) min_ent_len_7="CHECKED";
    else if (params.getVal("min_ent_len").equals("8")) min_ent_len_8="CHECKED";
    else if (params.getVal("min_ent_len").equals("9")) min_ent_len_9="CHECKED";
    else if (params.getVal("min_ent_len").equals("10")) min_ent_len_10="CHECKED";
    else min_ent_len_9="CHECKED";

    String min_corr_ent_len_5=""; String min_corr_ent_len_6=""; String min_corr_ent_len_7=""; String min_corr_ent_len_8="";
    if (params.getVal("min_corr_ent_len").equals("5")) min_corr_ent_len_5="CHECKED";
    else if (params.getVal("min_corr_ent_len").equals("6")) min_corr_ent_len_6="CHECKED";
    else if (params.getVal("min_corr_ent_len").equals("7")) min_corr_ent_len_7="CHECKED";
    else if (params.getVal("min_corr_ent_len").equals("8")) min_corr_ent_len_8="CHECKED";
    else min_corr_ent_len_6="CHECKED";

    String htm=
    ("<FORM NAME=\"mainform\" METHOD=POST ACTION=\""+response.encodeURL(SERVLETNAME)+"\" ENCTYPE=\"multipart/form-data\">\n")
    +("<TABLE WIDTH=\"100%\"><TR><TD><H1>"+APPNAME+"</H1></TD>\n")
    +("<TD WIDTH=\"70%\" VALIGN=\"middle\"><B>- NextMove LeadMine molecular entity recognition</B></TD>\n")
    +("<TD ALIGN=RIGHT>\n")
    +("<BUTTON TYPE=BUTTON onClick=\"void window.open('"+response.encodeURL(SERVLETNAME)+"?help=TRUE','helpwin','width=600,height=400,scrollbars=1,resizable=1')\"><B>Help</B></BUTTON>\n")
    +("<BUTTON TYPE=BUTTON onClick=\"go_demo(this.form)\"><B>Demo</B></BUTTON>\n")
    +("<BUTTON TYPE=BUTTON onClick=\"window.location.replace('"+response.encodeURL(SERVLETNAME)+"')\"><B>Reset</B></BUTTON>\n")
    +("</TD></TR></TABLE>\n")
    +("<HR>\n");

    htm+=
     ("<INPUT TYPE=HIDDEN NAME=\"txt2mol\">\n")
    +("<TABLE WIDTH=100% CELLPADDING=5 CELLSPACING=5>\n")
    +("<TR BGCOLOR=\"#CCCCCC\">\n")
    +("<TD WIDTH=\"60%\" VALIGN=TOP><B>Inputs:</B><BR>\n")
    +("upload:<INPUT TYPE=\"FILE\" NAME=\"ifile\">\n")
    +("<INPUT TYPE=CHECKBOX NAME=\"file2txt\" VALUE=\"CHECKED\" "+params.getVal("file2txt")+">file2txt<BR>or paste... \n")
    +("<TEXTAREA NAME=\"intxt\" ROWS=12 COLS=60 WRAP=OFF>"+params.getVal("intxt")+"</TEXTAREA>\n")
    +("</TD>\n")
    +("<TD VALIGN=TOP>\n")
    +("<TABLE>\n")
    +("<TR><TD COLSPAN=\"2\"><B>Configuration:</B></TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Spellcorrect:</TD><TD><INPUT TYPE=CHECKBOX NAME=\"spellcorrect\" VALUE=\"CHECKED\" "+params.getVal("spellcorrect")+"></TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Max_corrected_dist:</TD><TD><INPUT TYPE=RADIO NAME=\"max_corr_dist\" VALUE=\"1\" "+max_corr_dist_1+">1\n")
    +("<INPUT TYPE=RADIO NAME=\"max_corr_dist\" VALUE=\"2\" "+max_corr_dist_2+">2</TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Min_entity_len:</TD><TD><INPUT TYPE=RADIO NAME=\"min_ent_len\" VALUE=\"7\" "+min_ent_len_7+">7\n")
    +(" <INPUT TYPE=RADIO NAME=\"min_ent_len\" VALUE=\"8\" "+min_ent_len_8+">8\n")
    +(" <INPUT TYPE=RADIO NAME=\"min_ent_len\" VALUE=\"9\" "+min_ent_len_9+">9\n")
    +(" <INPUT TYPE=RADIO NAME=\"min_ent_len\" VALUE=\"10\" "+min_ent_len_10+">10</TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Min_corrected_entity_len:</TD><TD><INPUT TYPE=RADIO NAME=\"min_corr_ent_len\" VALUE=\"6\" "+min_corr_ent_len_6+">6\n")
    +(" <INPUT TYPE=RADIO NAME=\"min_corr_ent_len\" VALUE=\"7\" "+min_corr_ent_len_7+">7\n")
    +(" <INPUT TYPE=RADIO NAME=\"min_corr_ent_len\" VALUE=\"8\" "+min_corr_ent_len_8+">8</TD></TR>\n")
    +("<TR><TD COLSPAN=\"2\"><B>Output:</B></TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Mode:</TD><TD><INPUT TYPE=CHECKBOX NAME=\"out_view\" VALUE=\"CHECKED\" "+params.getVal("out_view")+">view\n")
    +("<INPUT TYPE=CHECKBOX NAME=\"out_batch\" VALUE=\"CHECKED\" "+params.getVal("out_batch")+">batch</TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Include NERless:</TD><TD><INPUT TYPE=CHECKBOX NAME=\"include_nerless\" VALUE=\"CHECKED\" "+params.getVal("include_nerless")+"></TD></TR>\n")
    +("<TR><TD COLSPAN=\"2\"><B>Misc:</B></TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Depict</TD><TD><INPUT TYPE=CHECKBOX NAME=\"depict\" VALUE=\"CHECKED\" "+params.getVal("depict")+"></TD></TR>\n")
    +("<TR><TD ALIGN=\"right\">Verbose</TD><TD><INPUT TYPE=CHECKBOX NAME=\"verbose\" VALUE=\"CHECKED\" "+params.getVal("verbose")+"></TD></TR>\n")
    +("</TABLE>\n")
    +("</TD>\n")
    +("</TR>\n")
    +("<TR>\n")
    +("<TD COLSPAN=\"2\" ALIGN=\"CENTER\">\n")
    +("<BUTTON TYPE=BUTTON onClick=\"go_txt2mol(this.form)\"><B>Go "+APPNAME+"</B></BUTTON>\n")
    +("</TD>\n")
    +("</TR>\n")
    +("</TABLE>\n")
    +("</FORM>\n");
    return htm;
  }
  /////////////////////////////////////////////////////////////////////////////
  private static void Name2Mol(MultipartRequest mrequest, HttpServletResponse response)
      throws IOException, ServletException
  {
     File fout = File.createTempFile(SERVLETNAME, "_out.tsv", null);
     OutputStream fos = new FileOutputStream(fout);
     BufferedWriter fob = new BufferedWriter(new OutputStreamWriter(fos));

     String orig_fname=mrequest.getOriginalFileName(mrequest.getParameter("ifile"));

     //errors.add("DEBUG: orig_fname = "+orig_fname);

     orig_fname = (orig_fname==null) ? "browser" : orig_fname;

    // Configuration:
    LeadMineConfig leadMineConfig = new LeadMineConfig(); //default
    SpellingCorrectorConfig spellingCorrectorConfig = leadMineConfig.getSpellingCorrectorConfig();

    List<CfxDictionary> leadMineDictionaries = leadMineConfig.getDictionaries();

    if (params.isChecked("spellcorrect")) {
      for (CfxDictionary cfxd: leadMineDictionaries) { // Modify per dictionary:
        cfxd.setCaseSensitive(false);
        cfxd.setUseSpellingCorrection(true);
        cfxd.setMaxCorrectionDistance(Integer.parseInt(params.getVal("max_corr_dist")));
        cfxd.setMinimumEntityLength(Integer.parseInt(params.getVal("min_ent_len")));
        cfxd.setMinimumCorrectedEntityLength(Integer.parseInt(params.getVal("min_corr_ent_len")));
      }
    }

    // Instantiate LeadMine:
    LeadMine leadMine = new LeadMine(leadMineConfig);

    // Check configuration:
    errors.add("LeadMine version: "+leadMine.getVersion());
    if (params.isChecked("verbose")) {
      errors.add("LeadMine lookBehindDepth: "+spellingCorrectorConfig.getExhaustiveLookBehindDepth());
      Set<String> entityTypes = leadMineConfig.getExpectedEntityTypes();
      for (String entityType: entityTypes) {
        errors.add("LeadMine entityType: "+entityType);
      }
      for (CfxDictionary cfxd: leadMineDictionaries) {
        errors.add("LeadMine dictionary: "+cfxd.getSource());
        errors.add("case_sensitive="+cfxd.isCaseSensitive());
        errors.add("spellcorrect="+cfxd.useSpellingCorrection());
        errors.add("max_correction_distance="+cfxd.getMaxCorrectionDistance());
        errors.add("min_entity_length="+cfxd.getMinimumEntityLength());
        errors.add("min_corrected_entity_length="+cfxd.getMinimumCorrectedEntityLength());
      }
    }

    TsvOutputBuilder tsvOutputBuilder = new TsvOutput.TsvOutputBuilder();
    tsvOutputBuilder = tsvOutputBuilder.withResolvers(leadMineConfig.getNameResolvers());
    //tsvOutputBuilder = tsvOutputBuilder.withCols(TsvOutput.getCliCols());
    //tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.DocName);
    //tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.SectionType)
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.OriginalText);
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.EntityType);
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.BegIndex);
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.EntityText);
    
    if (params.isChecked("spellcorrect"))
    {
      tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.PossiblyCorrectedText);
      tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.CorrectionDistance);
    }
    tsvOutputBuilder = tsvOutputBuilder.withCol(TsvOutput.TsvOutputColumn.ResolvedForm);

    TsvOutput tsvOut = tsvOutputBuilder.build();
    fob.write(tsvOut.getTsvHeader());
    fob.flush();

    // Requires that the NextMove cdkdepict.war webapp is deployed.
    String cdkdepict_url=(PROXY_PREFIX+"/cdkdepict/depict/cow/png");

    int fixcount=0;
    int n_ner=0;
    int n_ner_less=0;
    String thtm="<TABLE WIDTH=\"100%\" BORDER><TH>i_input</TH><TH>InputString</TH>\n";
    String[] tags = Pattern.compile("\\t").split(tsvOut.getTsvHeader());
    int j_smi=0;
    int j=0;
    for (String tag: tags) {
      ++j;
      thtm+=("<TH>"+tag+"</TH>");
      if (tag.equals("ResolvedForm")) j_smi=j;
    }

    for (int i_txt=0;i_txt<texts.size();++i_txt)
    {
      String intext=texts.get(i_txt);
      EntityCollector collector = leadMine.findEntities(intext);
      List<Entity> entities = collector.getEntities();
      String rhtm="";
      if (entities.size()==0) {
        if (params.isChecked("include_nerless"))
        {
          rhtm+=("<TR><TD ALIGN=\"center\">"+(i_txt+1)+"</TD>");
          rhtm+=("<TD ALIGN=\"center\" BGCOLOR=\"white\">"+intext+"</TD>");
          for (String tag: tags)
            rhtm+=("<TD ALIGN=\"center\" BGCOLOR=\"white\">~</TD>");
          rhtm+=("</TR>\n");
        }
      }
      else
      {
        String[] lines = Pattern.compile("[\\n\\r]").split(tsvOut.generateTsvString(collector, orig_fname));
        rhtm+=("<TR><TD ALIGN=\"center\" ROWSPAN=\""+lines.length+"\">"+(i_txt+1)+"</TD>");
        for (String line: lines) {
          rhtm+=("<TD ALIGN=\"center\" BGCOLOR=\"white\">"+intext+"</TD>");
          String[] vals = Pattern.compile("\\t").split(line);
          j=0;
          for (String val: vals) {
            ++j;
            if (params.isChecked("depict") && j_smi>0 && j==j_smi)
              val = ("<IMG HEIGHT=\"80\" SRC=\""+cdkdepict_url+"?smi="+URLEncoder.encode(val,"UTF-8")+"\">");
            rhtm+=("<TD ALIGN=\"center\" BGCOLOR=\"white\">"+val+"</TD>");
          }
          while (j<tags.length) {
            ++j;
            rhtm+=("<TD ALIGN=\"center\" BGCOLOR=\"white\">~</TD>");
          }
          rhtm+=("</TR>\n");
        }

        tsvOut.outputTsv(collector, fos, orig_fname); //1-line/entity, "" for none
        n_ner+=entities.size();
      }
      thtm+=rhtm;
      if (entities.size()==0) {
        errors.add("NOTE: NERless: ["+i_txt+"] \""+intext+"\"");
        ++n_ner_less;
      }
    }
    thtm+="</TABLE>";

    fob.close();
    fos.close();

    int n_ner_ed = texts.size() - n_ner_less;

    outputs.add("<H2>Results:</H2>");
    outputs.add("<BLOCKQUOTE><B>Inputs:</B> "+texts.size());
    outputs.add("<B>Inputs NERed:</B> "+n_ner_ed);
    outputs.add("<B>Inputs NERless:</B> "+n_ner_less);
    outputs.add("<B>NER_Total:</B> "+n_ner+"</BLOCKQUOTE>");

    if (params.isChecked("out_view")) {
      outputs.add(thtm);
    }

    String fname=(SERVLETNAME+"_out.tsv");
    if (params.isChecked("out_batch")) {
      outputs.add("&nbsp;"+
        "<FORM METHOD=\"POST\" ACTION=\""+response.encodeURL(SERVLETNAME)+"\">\n"+ "<INPUT TYPE=HIDDEN NAME=\"downloadfile\" VALUE=\""+fout.getAbsolutePath()+"\">\n"+
        "<INPUT TYPE=HIDDEN NAME=\"fname\" VALUE=\""+fname+"\">\n"+
        "<BUTTON TYPE=BUTTON onClick=\"this.form.submit()\">"+
        "download "+fname+" ("+file_utils.NiceBytes(fout.length())+")</BUTTON></FORM>\n");
    }

    PrintWriter out_log=new PrintWriter(new BufferedWriter(new FileWriter(logfile,true)));
    out_log.printf("%s\t%s\t%d\n", datestr, REMOTEHOST, texts.size());
    out_log.close();

    return;
  }
  /////////////////////////////////////////////////////////////////////////////
  private static String JavaScript()
  {
    String js="var demotxt='';";
    for (String str: DEMOLINES)
      js+=("demotxt+='"+str+"\\n';\n");
    js+=(
"function checkform(form)\n"+
"{\n"+
"  if (form.txt2mol.value && !form.intxt.value && !form.ifile.value) {\n"+
"    alert('ERROR: No input texts specified');\n"+
"    return false;\n"+
"  }\n"+
"  return true;\n"+
"}\n"+
"function go_init(form)"+
"{\n"+
"  form.file2txt.checked=true;\n"+
"  form.spellcorrect.checked=false;\n"+
"  form.out_batch.checked=true;\n"+
"  form.out_view.checked=true;\n"+
"  form.include_nerless.checked=true;\n"+
"  form.depict.checked=true;\n"+
"  for (i=0;i<form.max_corr_dist.length;++i)\n"+ //radio
"    if (form.max_corr_dist[i].value=='1')\n"+
"      form.max_corr_dist[i].checked=true;\n"+
"  for (i=0;i<form.min_ent_len.length;++i)\n"+ //radio
"    if (form.min_ent_len[i].value=='9')\n"+
"      form.min_ent_len[i].checked=true;\n"+
"  for (i=0;i<form.min_corr_ent_len.length;++i)\n"+ //radio
"    if (form.min_corr_ent_len[i].value=='7')\n"+
"      form.min_corr_ent_len[i].checked=true;\n"+
"  form.verbose.checked=false;\n"+
"  form.intxt.value='';\n"+
"  var i;\n"+
"}\n"+
"\n"+
"function load_demotxt(form) {\n"+
"  form.intxt.value=demotxt;\n"+
"}\n"+
"function go_demo(form) {\n"+
"  go_init(form);\n"+
"  load_demotxt(form);\n"+
"  form.txt2mol.value='TRUE'\n"+
"  form.submit()\n"+
"}\n"+
"function go_txt2mol(form) {\n"+
"  form.txt2mol.value='TRUE'\n"+
"  if (!checkform(form)) return;\n"+
"  form.submit()\n"+
"}\n");
    return(js);
  }
  /////////////////////////////////////////////////////////////////////////////
  private static String HelpHtm()
  {
    return("<H2>"+APPNAME+" Help</H2>\n"+
    "<P>\n"+
    "NextMove LeadMine chemical entity recognition, name to molecule.\n"+
    "This web application converts texts to molecules using\n"+
    "NextMove LeadMine and its Java API.\n"+
    "<P>\n"+
    "In this app, each input line is treated as a separate document.\n"+
    "<P>\n"+
    "<B>Chemical mention entity types:</B>\n"+
    "<UL>\n"+
    "<LI><B>M</B>: systematically named molecule\n"+
    "<LI><B>E</B>: element or mono-atomic name\n"+
    "<LI><B>D</B>: dictionary molecule name\n"+
    "<LI><B>C</B>: Chemical Abstracts Service number\n"+
    "<LI><B>R</B>: pharmaceutical registry number\n"+
    "<LI><B>A</B>: atomic (single atom) substituent prefix\n"+
    "<LI><B>P</B>: polyatomic substituent prefix\n"+
    "<LI><B>Y</B>: polymer\n"+
    "<LI><B>F</B>: formula\n"+
    "<LI><B>G</B>: generic term\n"+
    "</UL>\n"+
    "<P>\n"+
    "Depictions by <A HREF=\"https://cdk.github.io/\">CDK</A> via NextMove cdkdepict webapp.\n"+
    "<P>\n"+
    "<P>Sincere thanks to NextMove Software for their excellent software and support.\n"+
    "<P>\n"+
    "<UL>\n"+
    "<LI>Demo text from ClinicalTrials.gov.\n"+
    "<LI>Input limit N_MAX = "+N_MAX+"\n"+
    "</UL>\n"+
    "<P>\n"+
    "<B>References:</B>\n"+
    "<UL>\n"+
    "<LI><A HREF=\"http://nextmovesoftware.com\">NextMove Software</A>\n"+
    "</UL>\n"+
    "<P>\n"+
    "webapp author: Jeremy Yang\n"
    );
  }
  /////////////////////////////////////////////////////////////////////////////
  public void init(ServletConfig conf) throws ServletException
  {
    super.init(conf);
    CONTEXT=getServletContext();        // inherited method
    CONTEXTPATH=CONTEXT.getContextPath();
    try { APPNAME=conf.getInitParameter("APPNAME"); }
    catch (Exception e) { APPNAME=this.getServletName(); }
    UPLOADDIR=conf.getInitParameter("UPLOADDIR");
    if (UPLOADDIR==null)
      throw new ServletException("Please supply UPLOADDIR parameter.");
    LOGDIR=conf.getInitParameter("LOGDIR")+CONTEXTPATH;
    if (LOGDIR==null) LOGDIR="/tmp"+CONTEXTPATH+"_logs";
    try { N_MAX=Integer.parseInt(conf.getInitParameter("N_MAX")); }
    catch (Exception e) { N_MAX=100; }
    PROXY_PREFIX=((conf.getInitParameter("PROXY_PREFIX")!=null)?conf.getInitParameter("PROXY_PREFIX"):"");
  }
  /////////////////////////////////////////////////////////////////////////////
  public void doGet(HttpServletRequest request, HttpServletResponse response)
    throws IOException, ServletException
  {
    doPost(request, response);
  }
  /////////////////////////////////////////////////////////////////////////////
  public final static String[] DEMOLINES = new String[]{
"18F-FACBC Radiotracer",
"2% Lidocaine with 1:100,000 epinephrine",
"50,000 IU vitamin D3",
"80mg atorvastatin",
"acetaminophen/codeine and ibuprofen",
"Acetylsalicylic acid (Aspirin)",
"AMG 386 30 mg/kg, Capecitabine and Lapatinib",
"Amoxil® 500mg/5mL powder for oral suspension",
"Amphotericin B Lipid emulsion",
"Antipyrine and Benzocaine otic solution",
"atorvastatin (Lipitor)",
"Brincidofovir",
"Candesartan cilexetil 16mg",
"Cetuximab and FOLFOX",
"crestor(Rosuvastatin)",
"Cyclophosphamide, Doxorubicin, Vincristine, Prednisolone.",
"Dexedrine",
"Dextromethorphan and Caffeine",
"Dietary Supplement: LifePak Nano",
"DTI-0009",
"Efavirenz 600 mg",
"Escitalopram Matching Placebo",
"Etonogestrel containing contraceptive vaginal ring (ENG-CVR)",
"Faldaprevir",
"FOLFIRINOX (oxaliplatin, leucovorin, irinotecan)",
"gemcitabine; irinotecan; leucovorin; 5-fluorouracil; oxaliplatin",
"Glyburide and Metformin Hydrochloride Tablets 5 mg/500 mg",
"GSK256073 5mg",
"Hexylaminolevulinate cream",
"Homeopathic medication Plumbum metallicum",
"ibuprofen 600 mg tid",
"Lansoprazole Capsules",
"Leucovorin calcium",
"Losartan (Control)",
"Maltodextrin (Placebo)",
"Mannitol (20%)",
"Metformin hydrochloride (Glucophage®)",
"MK2578 1mcg for every 600 Units (U) of Epogen® (epoetin alfa) received per week at",
"Baseline",
"montelukast 4 mg granule",
"Morphine Extended Release",
"Nalbuphine Sebacate",
"Nevirapine (NVP): Infant extended",
"Niacin/simvastatin compared to simvastatin alone at 2 doses",
"Oral Omega-3-acid ethyl esters",
"oxytocin",
"Paracetamol drops",
"Paroxetine hydrochloride 40 mg tablet",
"Peg Interferon alpha2b + Ribavirin",
"Penicillin versus cefuroxim per os",
"Picosulfate sodium, magnesium oxide, citric acid",
"placebo comparator of N-acetylcysteine",
"Placebo Sodium Phenylbutyrate plus active cholecalciferol",
"Pritor + Omacor",
"R89674 (generic name not yet established)",
"Real time enoxaparin dose adjustment",
"Rocuronium Injection",
"Saquinavir-sgc",
"SB-480848",
"Simethicone 80 MG",
"Stiripentol",
"Sufentanil (R30730, brand name Sufenta)",
"Vincristine, Irinotecan",
"vitamin D3 5000 IU",
"Voltaren® Gel"
  };
}
