#!/usr/bin/env qore

# sqlite3 test script

%require-our
%enable-all-warnings

%disable-warning excess-args

our $o;

const opts = {
    "help"    : "h,help",
    "db"      : "d,db=s",
    "enc"     : "e,encoding=s",
    "v"       : "v"
};

sub usage() {
    printf("usage: %s [options]
 -h,--help          this help text
 -d,--db=ARG        set database name - to create sqlite3 file name or \":memory:\"
                    Warning: the database will not be cleaned up.
 -e,--encoding=ARG  set database character set encoding (i.e. \"utf8\")
 -v                 verbose. It prints the results out too\n",
       basename($ENV."_"));
    exit();
}


sub cout($var)
{
    if ($o.v)
        printf("    result: %N\n\n", $var);
}

my $g = new GetOpt(opts);
$o = $g.parse(\$ARGV);

if ($o.help)
    usage();

if (!strlen($o.db)) {
    $o.db = ":memory:";
}
if (elements $ARGV)
{
    stderr.printf("excess arguments on command-line (%n): -h for help\n", $ARGV);
    exit(1);
}

my Datasource $ds("sqlite3", "", "", $o.db, $o.enc);

if ($o.v) printf("Sqlite3 version used: %s\n", $ds.getServerVersion());


if ($o.v) printf("Test 1 - create table foo\n");
my auto $result = $ds.exec("create table foo (
                id integer primary key autoincrement,
                txt text,
                int integer,
                dbl real,
                blb blob,
                bol integer
            )");
cout($result);


if ($o.v) printf("Test 2 - Create view v_foo with statement containing object names as parameters\n");
$result = $ds.exec("create view %s as select * from %s;", "v_foo", "foo");
cout($result);


if ($o.v) printf("Test 3 - Start a transaction (should fail)\n");
try {
    $result = $ds.beginTransaction();
    cout($result);
} catch (hash<ExceptionInfo> $ex) {
    if ($o.v) printf("   Caught exception: %N\n\n", $ex);
}


if ($o.v) printf("Test 4 - inserts\n");
my $f = new File("utf8");
$f.open2(get_script_dir() + "blob.png", O_RDONLY);
my $data = $f.readBinary(-1);
delete $f;
my $bool;
for (my $i = 0; $i < 50; $i++)
{
    $bool = ($i % 2 == 0);
    $result = $ds.exec("insert into foo (txt, int, dbl, blb, bol) values (%v, %v, %v, %v, %v)",
                        "lorem ipsum", rand(), rand()/1.0, $data, $bool);
    cout($result);
}

if ($o.v) printf("Test 5 - commit\n");
$result = $ds.commit();
cout($result);

if ($o.v) printf("Test 6 - select from table\n");
$result = $ds.select("select * from foo;");
cout($result);

if ($o.v) printf("Test 7 - select from view with replace-bind in the statement\n");
$result = $ds.select("select * from %s;", "v_foo");
cout($result);

if ($o.v) printf("Test 8 - select rows from table\n");
$result = $ds.selectRows("select * from foo;");
cout($result);

if ($o.v) printf("Test 9 - select rows from view with replace-bind in the statement\n");
$result = $ds.selectRows("select * from %s;", "v_foo");
cout($result);

if ($o.v) printf("Test 10 - select from table via exec() with bindings\n");
$result = $ds.exec("select * from foo where %s < %v", "id", 6);
cout($result);

if ($o.v) printf("Test 11 - select from table via select() with bindings\n");
$result = $ds.select("select * from foo where %s < %v", "id", 6);
cout($result);

if ($o.v) printf("Test 11 - select from table via selectRows() with bindings\n");
$result = $ds.selectRows("select * from foo where %s < %v", "id", 6);
cout($result);

if ($o.v) printf("Test 12 - pragma call - with returning value\n");
$result = $ds.exec("PRAGMA encoding;");
cout($result);
$result = $ds.exec("PRAGMA table_info(%s);", "foo");
cout($result);

if ($o.v) printf("Test 13 - pragma call - settings with no returns\n");
$result = $ds.exec("PRAGMA count_changes=%s;", 1);
cout($result);


if ($o.v) printf("Test 14 - execRaw() calls\n");
# OK
$result = $ds.execRaw("select * from foo where id = 6");
cout($result);
# error
try {
    $result = $ds.execRaw("select * from foo where %s < %v", "id", 6);
} catch (hash<ExceptionInfo> $ex) {
    if ($ex.err != "DBI:SQLITE3:EXECRAW" || $ex.desc != 'sqlite3 error: near "%": syntax error') {
        rethrow;
    }
}

$ds.rollback();
printf("All test passed\n");
