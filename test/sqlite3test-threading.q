#!/usr/bin/env qore

# sqlite3 test script

%require-our
%enable-all-warnings


our $o;

const opts = 
    ( "help"    : "h,help",
      "db"      : "d,db=s",
      "enc"     : "e,encoding=s"
 );

sub usage()
{
    printf("usage: %s [options]
 -h,--help          this help text
 -d,--db=ARG        set database name - to create sqlite3 file name or \":memory:\"
                    Warning: the database will not be cleaned up.
 -e,--encoding=ARG  set database character set encoding (i.e. \"utf8\")\n",
       basename($ENV."_"));
    exit();
}

my $g = new GetOpt(opts);
$o = $g.parse(\$ARGV);

if ($o.help)
    usage();

if (!strlen($o.db))
{
    stderr.printf("set the DB file with -d, etc (-h for help)\n");
    exit(1);
}
if (elements $ARGV)
{
    stderr.printf("excess arguments on command-line (%n): -h for help\n", $ARGV);
    exit(1);
}


# our $ds = new Datasource("sqlite3", "", "", "test.db");
our $ds = new DatasourcePool("sqlite3", "", "", $o.db, $o.enc, "", 5, 20);
our $threadCount = 100;

my $createStmt = "
    CREATE TABLE table1 (
        id integer primary key autoincrement,
        col1 text,
        col2 number,
        col3 text
    )
    ";
        
our $insertStmt = "insert into table1 (col1, col2, col3)
                    values (%v, %v, %v)";

our $selectStmt = "select * from table1 where id < %v";



sub insertWorker()
{
    printf("Starting insertWorker. Instances %d\n", $threadCount);
    for(my $i = 0; $i < $threadCount; ++$i)
        background insertExec();
    printf("End insertWorker\n");
}

sub insertExec()
{
    printf("Starting insertExec\n");
#     $ds.beginTransaction();
    try
    {
#         $ds.beginTransaction();
        my $result = $ds.exec($insertStmt, rand(), "Lorem Ipsum", rand());
        $ds.commit();
    }
    catch (my $ex)
    {
        printf("Insert failed (rollback will be performed): %N\n", $ex);
        $ds.rollback();
    }

    printf("End insertExec\n");
}


sub selectWorker()
{
    printf("Starting selectWorker. Instances %d\n", $threadCount);
    for(my $i = 0; $i < $threadCount; ++$i)
        background selectExec();
    printf("End selectWorker\n");
}

sub selectExec()
{
    printf("Starting selectExec\n");
    my $result = $ds.select($selectStmt, (rand()%1000)); #$threadCount+1);
    printf("Selected count: %d\n", elements $result."id");
    printf("End selectExec\n");
}


try
{
    $ds.exec($createStmt);
    printf("table created\n");
}
catch ($ex)
    printf("table is not created: %N\n", $ex);
$ds.commit();

background insertWorker();
usleep(300ms);
background selectWorker();
usleep(300ms);
background selectWorker();

$ds.commit();
