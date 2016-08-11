<?hh // decl
namespace Usox\HaDb;

function pg_connect($connection_string) {
	return \Usox\HaDb\Test\DatabasePostgresTest::$functions->pg_connect($connection_string);
}
function pg_query($connection, $query_string) {
	return \Usox\HaDb\Test\DatabasePostgresTest::$functions->pg_query($connection, $query_string);
}
function pg_fetch_assoc(resource $pgsql_resource) {
	return \Usox\HaDb\Test\DatabasePostgresTest::$functions->pg_fetch_assoc($pgsql_resource);
}
function pg_escape_string($connection, $string) {
	return \Usox\HaDb\Test\DatabasePostgresTest::$functions->pg_escape_string($connection, $string);
}
function pg_fetch_result(resource $resource, int $row_number, string $field_name) {
	return \Usox\HaDb\Test\DatabasePostgresTest::$functions->pg_fetch_result($resource, $row_number, $field_name);
}

namespace Usox\HaDb\Test;

use Usox\HaDb\DatabaseConfigInterface;
use Usox\HaDb\DatabasePostgres;
use Usox\HaDb\Exception;
use \Mockery as m;

class DatabasePostgresTest extends \PHPUnit_Framework_TestCase {

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

		$this->database = new DatabasePostgres($this->configuration);
	}

	public function testGetConnectionThrowsExceptionOnPgConnectFailure(): void {
		$hostname = 'never.mind';

		$this->setExpectedException(
			Exception\DatabaseInitializationException::class,
			'Connection to host '.$hostname.' failed'
		);

		self::$functions
			->shouldReceive('pg_connect')
			->once()
			->andReturn(null);

		$this->configuration
			->shouldReceive('getHost')
			->once()
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
			->shouldReceive('pg_connect')
			->once()
			->with(
				sprintf(
					'host=%s port=%d dbname=%s user=%s password=%s',
					$hostname,
					$port,
					$name,
					$user,
					$password
				)
			)
			->andReturn($this->connection_resource);

		// UNSAFE
		$this->assertSame(
			$this->connection_resource,
			$this->database->getConnection()
		);
	}

	public function testQueryFailsButIncrementsQueryCount(): void {
		$this->createConnectionExpectation();
		$query_count_before = $this->database->getQueryCount();

		$query = 'this is supposed to fail';

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andThrow(Exception\QueryFailedException::class);

		try {
			$this->database->query($query);
		} catch (Exception\QueryFailedException $e) {
		}

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
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andReturn($result);

		$this->assertSame(
			$result,
			$this->database->query($query)
		);
	}

	public function testTransactionBeginSendsBeginQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, 'BEGIN')
			->andReturn(curl_init());

		$this->database->transactionBegin();
	}

	public function testTransactionCommitSendsCommitQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, 'COMMIT')
			->andReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testTransactionRollbackSendsRollbackQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, 'COMMIT')
			->andReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testGetNextResultReturnsNullOnError(): void {
		$this->connection_resource = curl_init();

		self::$functions
			->shouldReceive('pg_fetch_assoc')
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
			->shouldReceive('pg_fetch_assoc')
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

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('pg_fetch_result')
			->once()
			->with($this->connection_resource, 0, 'exists')
			->andReturn('t');

		$this->assertTrue(
			$this->database->exists($query)
		);
	}

	public function testExistsReturnsFalseIfItemDoesNotExists(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT EXISTS(meh)';

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('pg_fetch_result')
			->once()
			->with($this->connection_resource, 0, 'exists')
			->andReturn('f');

		$this->assertFalse(
			$this->database->exists($query)
		);
	}

	public function testCountReturnsNumberOfRows(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT COUNT(*)';
		$count = 123;

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('pg_fetch_result')
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

		$query = 'SELECT LASTVAL()';
		$id = 666;

		self::$functions
			->shouldReceive('pg_query')
			->once()
			->with($this->connection_resource, $query)
			->andReturn($this->connection_resource);

		self::$functions
			->shouldReceive('pg_fetch_result')
			->once()
			->with($this->connection_resource, 0, 'lastval')
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
			->shouldReceive('pg_connect')
			->once()
			->andReturn($this->connection_resource);
	}
}
