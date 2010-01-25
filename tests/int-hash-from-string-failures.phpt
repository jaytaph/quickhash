--TEST--
Test for failure conditions for loadFromString.
--INI--
extension=quickhash.so
xdebug.default_enable=0
--FILE--
<?php
echo "\nWrong params: \n";
try
{
	$hash = QuickHashIntSet::loadFromString();
}
catch( Exception $e )
{
	echo $e->getMessage(), "\n";
}

try
{
	$hash = QuickHashIntSet::loadFromString( 1024, 2, 'stuff' );
}
catch( Exception $e )
{
	echo $e->getMessage(), "\n";
}

try
{
	$hash = QuickHashIntSet::loadFromString( 1024, 'stuff' );
}
catch( Exception $e )
{
	echo $e->getMessage(), "\n";
}

try
{
	$hash = QuickHashIntSet::loadFromString( new StdClass );
}
catch( Exception $e )
{
	echo $e->getMessage(), "\n";
}

echo "\nWrong size: \n";
try
{
	$contents = file_get_contents( dirname( __FILE__ ) . "/broken-file.set" );
	$hash = QuickHashIntSet::loadFromString( $contents );
}
catch( Exception $e )
{
	echo $e->getMessage(), "\n";
}
?>
--EXPECT--

Wrong params: 
QuickHashIntSet::loadFromString() expects at least 1 parameter, 0 given
QuickHashIntSet::loadFromString() expects at most 2 parameters, 3 given
QuickHashIntSet::loadFromString() expects parameter 2 to be long, string given
QuickHashIntSet::loadFromString() expects parameter 1 to be string, object given

Wrong size: 
QuickHashIntSet::loadFromString(): String is in the wrong format (not a multiple of 4 bytes)
