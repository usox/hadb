[![Build Status](https://travis-ci.org/usox/hadb.svg?branch=master)](https://travis-ci.org/usox/hadb)

HaDb - Hack DB
==============

Query sql databases using hack in strict mode - that's it.

Usage
-----

Just create a config class using the `\Usox\HaDb\DatabaseConfigInterface` and
instantiate HaDb;

```php
$database_config = new MyDatabaseConfig();

$hadb = Usox\HaDb\DatabaseAdapter::factory(
  $database_config
);
```

Supported Databases
-------------------

* PostgreSQL
* MySql
