#!/usr/bin/perl
#
# publish
# This Perl script basically converts one or more text files into a simple blog.
#
# (c) 2010 Carlos E. Torchia
# This software is licensed and to be distributed under the terms of the
# GNU General Public License version 2. Go to http://www.fsf.org/ for 
# details.
#
# To use this script, call it with the path to the config file
# $ publish .../blog.conf
# Note: it doesn't actually have to be called 'blog.conf'
#
# blog.conf is actually an HTML file that is to be a template for each 
# page in the blog. 
#
# In the blog.conf file, <!-- article --> indicates where the blog
# article goes, assuming the page being generated is an article. 
# Otherwise, it is where the index goes.
#
# NOTE: each article's filename MUST end in ".txt"
#
# The index is a list sorted in descending order by date of the
# articles in the blog.
#
# Each blog article has a title and a date (which is respectively the 
# filename and the date last modified as per the filesystem).
# In blog.conf, <!-- title --> gets replaced by the the article's title
# and <!-- date --> gets replaced by the article's date. Also
# <!-- home --> indicates a link that takes the reader back to the 
# index.
#
# To set the location of plain text articles, use:
# <!-- text DIR -->
# where DIR is the directory containing the articles.
#
# And:
# <!-- web DIR -->
# sets the directory where the HTML blog pages are to be
# stored.
#
# NOTE:  The EACH web and text directive MUST be on its OWN LINE.
#        The web and text directives SHALL NOT to be left in the final
#        product, for security reasons.
#

#
# get_directory_names($conf_filename)
# Gets the web and text directory names from the conf file
# Input: conf filename
#

sub	get_directory_names
{
  local($conf_filename)=($_[0]);	# the conf filename
  local($text_directory)=("");
  local($web_directory)=("");

  #
  # Open the configuration file
  #

  open(CONF_FILE,"<".$conf_filename)
    or die "publish: $conf_filename: cannot open: $!\n";

  #
  # Obtain the article and blog directories
  #

  while($_ = <CONF_FILE>) {

    #
    # Plain text article directory
    #

    if(/\<\!\-\- text (.+) \-\-\>/) {
      $text_directory=$1;
      $text_directory =~ s/([^\/])$/\1\//; # ensure it ends with /
    }

    #
    # Web directory for the blog
    #

    elsif(/\<\!\-\- web (.+) \-\-\>/) {
      $web_directory=$1;
      $web_directory =~ s/([^\/])$/\1\//;	# ensure it ends with /
    }

  }

  #
  # Close the conf file
  #

  close(CONF_FILE);

  #
  # Ensure that there is a web directory
  #

  if($web_directory eq "") {
    die "publish: $conf_filename: web directory not specified\n";
  }

  #
  # Return the directories
  #

  ($web_directory,$text_directory);
}

#
# get_text_articles(@text_article_directories)
# Gets the set of text articles
# Input: text article directory path
#

sub	get_text_articles
{

  local($textdir)=($_[0]);
  local(@text_articles)=();

  #
  # Put each file that is in each directory in the output list
  #

  if($textdir ne "") {
    opendir(DIR,$textdir)
      or die "publish: $dir: cannot open: $!\n";
    while($file=readdir(DIR)) {
      @text_articles=(@text_articles,$file) if $file =~ /\.txt$/;
    }
    closedir(DIR);
  }

  @text_articles;

}

# Gets the string representation of the time in seconds since epoch

sub get_date_str
{

  $date=$_[0];
  sprintf("%5d/%02d/%02d  %02d:%02d",
          (localtime($date))[5]+1900,
          (localtime($date))[4]+1,
          (localtime($date))[3],
          (localtime($date))[2],
          (localtime($date))[1]);
#          (localtime($date))[0]);

}

#
# write_web_file($webdir,$conf_filename,$web_filename,$body,$title,$date)
# Generates an HTML file based on 
#

sub	write_web_file
{

  local($webdir)=();
  local($conf_filename)=();
  local($body)=();
  local($title)=();
  local($date)=();
  local($web_filename)=();
  local($home)=('<a href="index.html">Home</a>');

  ($webdir,$conf_filename,$web_filename,$body,$title,$date)=@_; 

  $date_str=&get_date_str($date);

  open(WEB_FILE,">$webdir".$web_filename);
  open(CONF_FILE,"<$conf_filename");
  print "  $webdir"."$web_filename\n";
 
  #
  # Replace each tag in the conf file with the appropriate thing for
  # the web file.
  #

  while($_=<CONF_FILE>) {

    s/\<\!-- article --\>/$body/g;
    s/\<\!-- title --\>/$title/g;
    s/\<\!-- date --\>/$date_str/g;
    s/\<\!-- home --\>/$home/g;

    # And anything else...
    s/\<\!-- .* --\>//g;

    # Write it out
    print WEB_FILE $_;

  }

  close(WEB_FILE);
  close(CONF_FILE);

}

#
# get_title_date($textdir,$text_article)
# Gets the (title,date) pair for $textdir="path/.../", $text_article="name.txt"
#

sub	get_title_date
{
  local($textdir)=($_[0]);
  local($text_article)=($_[1]);
  local($title,$date)=();

  $title=$text_article;
  $title =~ s/\.txt$//;
  $date=(stat($textdir.$text_article))[9];

  # Return the title/date
  ($title,$date);
}

#
# get_body($text_filename)
# Converts file $text_filename into an HTML body. Returns $body.
#

sub	get_body
{
  local($body)=("");
  local($text_filename)=();
  $text_filename=$_[0];

  # Open plain text article file

  open(PLAIN_FILE,"<".$text_filename);

  #
  # Generate HTML for plain text and insert into a web file.
  #

  $body="";

  while($_=<PLAIN_FILE>) {

    chomp;
    $body=$body."$_ <br />\n";

  }

  # And close

  close(PLAIN_FILE);

  # Return the body

  $body;  

}

#
# get_web_filename($title,$num)
# Gets the web filename for a blog article.
# Does not include path
#

sub	get_web_filename
{

#  local($title)=($_[0]);
  local($num)=($_[1]);

#  $title =~ s/\s/\_/g;
#  $title =~ s/([^\w])//g;

  # Return filename
  sprintf("%04x.html",$num);

}

#
# get_excerpt($text_filename,$chars)
# Gets a an experpt of the fully specified file path with $chars 
# characters.
#

sub	get_excerpt
{
  local($text_filename,$chars)=(@_);
  local($line);

  open(EXCERPT_FILE,"<$text_filename");

  $line=<EXCERPT_FILE>.<EXCERPT_FILE>.<EXCERPT_FILE>;

  if(length($line)>$chars) {
    $line=substr($line,0,$chars);
    $line=$line.'...';
  }

  elsif ($_=<EXCERPT_FILE>) {
    $line=$line.'...';
  }

  close(EXCERPT_FILE);			# Close file like nice little boy

  $line;				# Return $line as output
}

#
# generate_index($textdir,$webdir,$conf_filename,@text_articles)
# Generates the index given the text articles
#

sub	generate_index
{

  local(@text_articles)=();
  local($webdir)=();
  local($conf_filename)=();
  local(@body_lines)=();
  local($line,$excerpt);
  local($title,$date)=(); 
  local($plain_filename)=();
  local($web_filename);
  local($num)=(0);		# Article ID

  ($textdir,$webdir,$conf_filename,@text_articles)=@_;

  for $plain_filename (@text_articles) {

    # Generate title, date

    ($title,$date)=&get_title_date($textdir,$plain_filename);

    # Generate HTML filename for article

    $num=$num+1;
    $web_filename=&get_web_filename($title,$num);

    # Generate article entry in the index

    $line="<i>".&get_date_str($date)."</i> - ".
          "<a href=\"$web_filename\">$title</a> ".
          " <br />\n".
          &get_excerpt($textdir.$plain_filename,200).
          " <br /><br />\n";
    

    # Add to body

    @body_lines=(@body_lines,$line);

  }

  #
  # Sort articles in descending chronological order
  #

  @body_lines=sort { $b cmp $a } @body_lines;

  #
  # Now we are ready to put it in a web file
  #

  write_web_file($webdir,$conf_filename,"index.html","@body_lines","Index",time);

}

#
# main program
#

sub	main
{

  #
  # Get location of conf file
  #

  if($#ARGV!=0) {
    die "Usage: publish CONF-FILE\n";
  }

  $conf_filename=$ARGV[0];

  #
  # Get the web blog and text article directory filenames from conf_file
  #

  ($webdir,$textdir)=&get_directory_names($conf_filename);

  $"="\n  ";

  print "web:\n  $webdir\n";
  print "text:\n  $textdir\n";

  #
  # Get all the article filenames
  #

  @text_articles=get_text_articles($textdir);
  print "Articles:\n  @text_articles\n";

  $"=' ';

  #
  # Generate the index HTML file
  #

  print "Index:\n";
  &generate_index($textdir,$webdir,$conf_filename,@text_articles);

  #
  # For each text article, there shall be a corresponding HTML article
  #

  $num=0;		# Article ID
  print "Web Articles:\n";

  for $text_article (@text_articles) {

    # Generate title, date, web filename

    ($title,$date)=&get_title_date($textdir,$text_article);
    $num++;
    $web_filename=&get_web_filename($title,$num);

    #
    # Generate HTML for plain text and insert into a web file.
    #

    $body=get_body($textdir.$text_article);

    &write_web_file($webdir,$conf_filename,$web_filename,$body,$title,$date);

  }

}

&main;
