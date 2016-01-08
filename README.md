HaDb - Hack DB
==============

Query sql databases using hack in strict mode - that's all.

Usage
-----

Just create a config class using the `\Usox\HaDb\DatabaseConfigInterface` and
instantiate HaDb;

	$database_config = new MyDatabaseConfig();

	$hadb = new \Usox\HaDb\DatabasePostgres(
		$database_configuration
	);

Supported Databases
-------------------

* PostgreSQL
