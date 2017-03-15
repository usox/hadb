<?hh // decl
namespace Usox\HaDb;

function mysql_connect(string $host, string $user, string $password) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_connect($host, $user, $password);
}
function mysql_select_db(string $db_name, ?resource $resource) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_select_db($db_name, $resource);
}
function mysql_query(string $query_string, resource $connection) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_query($query_string, $connection);
}
function mysql_fetch_assoc(resource $resource) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_fetch_assoc($resource);
}
function mysql_real_escape_string(string $string, resource $connection) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_real_escape_string($string, $connection);
}
function mysql_insert_id(resource $resource) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_insert_id($resource);
}
function mysql_result(resource $resource, int $row_number, string $field_name) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_result($resource, $row_number, $field_name);
}
function mysql_num_rows(resource $resource) {
	return \Usox\HaDb\Test\DatabaseMySqlTest::$functions->mysql_num_rows($resource);
}

namespace Usox\HaDb\Test;

use Usox\HaDb\DatabaseConfigInterface;
use Usox\HaDb\DatabaseMySql;
use Usox\HaDb\Exception;
use \Mockery as m;

class DatabaseMySqlTest extends \PHPUnit_Framework_TestCase {

	private ?DatabaseConfigInterface $configuration;

	private ?DatabasePostgres $database;

	private ?resource $connection_resource;

	public static mixed $functions;

	public function setUp(): void {
		$this->configuration = m::mock(DatabaseConfigInterface::class);

		self::$functions = m::mock();
		/**
		 * Hacky! As we can't create test double of type `resource`,
		 * we have to use a little trick
		 */
		$this->connection_resource = curl_init();

		$this->database = new DatabaseMySql($this->configuration);
	}

	public function testGetConnectionThrowsExceptionOnMySqlConnectFailure(): void {
		$hostname = 'never.mind';

		$this->setExpectedException(
			Exception\DatabaseInitializationException::class,
			'Connection to host '.$hostname.' failed'
		);

		self::$functions
			->shouldReceive('mysql_connect')
			->once()
			->andReturn(null);
		self::$functions
			->shouldReceive('mysql_select_db')
			->with('', null)
			->once()
			->andReturn(true);

		$this->configuration
			->shouldReceive('getHost')
			->twice()
			->andReturn($hostname);
		$this->configuration
			->shouldReceive('getPort')
			->once()
			->andReturn('');
		$this->configuration
			->shouldReceive('getName')
			->once()
			->andReturn('');
		$this->configuration
			->shouldReceive('getUser')
			->once()
			->andReturn('');
		$this->configuration
			->shouldReceive('getPassword')
			->once()
			->andReturn('');

		// UNSAFE
		$this->database->getConnection();
	}

	public function testGetConnectionReturnsResource(): void {
		$hostname = 'my-host';
		$port = 1337;
		$name = 'my-db';
		$user = 'db-user';
		$password = 'db-pass';

		$this->configuration
			->shouldReceive('getHost')
			->once()
			->andReturn($hostname);
		$this->configuration
			->shouldReceive('getPort')
			->once()
			->andReturn($port);
		$this->configuration
			->shouldReceive('getName')
			->once()
			->andReturn($name);
		$this->configuration
			->shouldReceive('getUser')
			->once()
			->andReturn($user);
		$this->configuration
			->shouldReceive('getPassword')
			->once()
			->andReturn($password);

		self::$functions
			->shouldReceive('mysql_connect')
			->once()
			->with(
				sprintf('%s:%d', $hostname, $port),
				$user,
				$password
			)
			->andReturn($this->connection_resource);
		self::$functions
			->shouldReceive('mysql_select_db')
			->with($name, $this->connection_resource)
			->once()
			->andReturn(true);

		$this->assertSame(
			$this->connection_resource,
			$this->database->getConnection()
		);
	}

	public function testQueryFailsButIncrementsQueryCount(): void {
		$this->setExpectedException(Exception\QueryFailedException::class);

		$this->createConnectionExpectation();
		$query_count_before = $this->database->getQueryCount();

		$query = 'this is supposed to fail';

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with($query, $this->connection_resource)
			->andReturn(false);

		$this->database->query($query);

		$this->assertSame(
			$query_count_before+1,
			$this->database->getQueryCount()
		);
	}

	public function testQueryReturnsResult(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT * from users where clue > 0';
		$result = curl_init();

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with($query, $this->connection_resource)
			->andReturn($result);

		$this->assertSame(
			$result,
			$this->database->query($query)
		);
	}

	public function testTransactionBeginSendsBeginQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with('START TRANSACTION', $this->connection_resource)
			->andReturn(curl_init());

		$this->database->transactionBegin();
	}

	public function testTransactionCommitSendsCommitQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with('COMMIT', $this->connection_resource)
			->andReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testTransactionRollbackSendsRollbackQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with('COMMIT', $this->connection_resource)
			->andReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testGetNextResultReturnsNullOnError(): void {
		$this->connection_resource = curl_init();

		self::$functions
			->shouldReceive('mysql_fetch_assoc')
			->once()
			->with($this->connection_resource)
			->andReturn(false);

		$this->assertNull(
			$this->database->getNextResult($this->connection_resource)
		);
	}

	public function testGetNextResultReturnsResult(): void {
		$this->connection_resource = curl_init();
		$result = ['key' => null];

		self::$functions
			->shouldReceive('mysql_fetch_assoc')
			->once()
			->with($this->connection_resource)
			->andReturn($result);

		$this->assertSame(
			$result,
			$this->database->getNextResult($this->connection_resource)
		);
	}

	public function testQuoteReturnsEscapedString(): void {
		$this->createConnectionExpectation();

		$query = 'my-fancy-query';
		$escaped_query = 'my-fancy-escaped-query';

		self::$functions
			->shouldReceive('mysql_real_escape_string')
			->once()
			->with($query, $this->connection_resource)
			->andReturn($escaped_query);

		$this->assertSame(
			$escaped_query,
			$this->database->quote($query)
		);
	}

	public function testExistsReturnsFalseIfItemDoesNotExists(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT 1';

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with($query, $this->connection_resource)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('mysql_num_rows')
			->once()
			->with($this->connection_resource)
			->andReturn(0);

		$this->assertFalse(
			$this->database->exists($query)
		);
	}

	public function testCountReturnsNumberOfRows(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT COUNT(*)';
		$count = 123;

		self::$functions
			->shouldReceive('mysql_query')
			->once()
			->with($query, $this->connection_resource)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('mysql_result')
			->once()
			->with($this->connection_resource, 0, 'count')
			->andReturn((string) $count);

		$this->assertSame(
			$count,
			$this->database->count($query)
		);
	}

	public function testGetLastInsertedIdReturnsId(): void {
		$this->createConnectionExpectation();

		$id = 666;

		self::$functions
			->shouldReceive('mysql_insert_id')
			->once()
			->with($this->connection_resource)
			->andReturn((int) $id);

		$this->assertSame(
			$id,
			$this->database->getLastInsertedId()
		);
	}

	private function createConnectionExpectation(): void {
		$hostname = 'my-host';
		$port = 1337;
		$name = 'my-db';
		$user = 'db-user';
		$password = 'db-pass';

		$this->configuration
			->shouldReceive('getHost')
			->once()
			->andReturn($hostname);
		$this->configuration
			->shouldReceive('getPort')
			->once()
			->andReturn($port);
		$this->configuration
			->shouldReceive('getName')
			->once()
			->andReturn($name);
		$this->configuration
			->shouldReceive('getUser')
			->once()
			->andReturn($user);
		$this->configuration
			->shouldReceive('getPassword')
			->once()
			->andReturn($password);
		self::$functions
			->shouldReceive('mysql_connect')
			->once()
			->andReturn($this->connection_resource);
		self::$functions
			->shouldReceive('mysql_select_db')
			->once()
			->andReturn(true);
	}
}
