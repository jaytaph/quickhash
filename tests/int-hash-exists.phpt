--TEST--
Timing tests for exists();
--INI--
extension=quickhash.so
xdebug.default_enable=0
--FILE--
<?php
//generate 200000 elements
$array = range( 0, 199999 );
$existingEntries = array_rand( array_flip( $array ), 180000 );
$testForEntries = array_rand( array_flip( $array ), 1000 );
$foundCount = 0;

echo "Creating set: ", microtime( true ), "\n";
$set = new QuickHashIntSet( 100000 );
echo "Adding elements: ", microtime( true ), "\n";
foreach( $existingEntries as $key )
{
	$set->add( $key );
}

echo "Doing 1000 tests: ", microtime( true ), "\n";
foreach( $testForEntries as $key )
{
	$foundCount += $set->exists( $key );
}
echo "Done, $foundCount found: ", microtime( true ), "\n";
?>
--EXPECTF--
Creating set: %f
Adding elements: %f
Doing 1000 tests: %f
Done, %d found: %f
