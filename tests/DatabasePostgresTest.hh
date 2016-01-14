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

class DatabasePostgresTest extends \PHPUnit_Framework_TestCase {

	private ?DatabaseConfigInterface $configuration;

	private ?DatabasePostgres $database;

	private ?resource $connection_resource;

	public static mixed $functions;

	public function setUp(): void {
		$this->configuration = $this->getMock(DatabaseConfigInterface::class);

		self::$functions = $this->getMockBuilder('SimpleStub')
			->setMethods(
				['pg_connect', 'pg_query', 'pg_fetch_assoc', 'pg_escape_string', 'pg_fetch_result']
			)
			->getMock();
		/**
		 * Hacky! As we can't create test double of type `resource`,
		 * we have to use a little trick
		 */
		$this->connection_resource = curl_init();

		$this->database = new DatabasePostgres($this->configuration);
	}

	public function testGetConnectionThrowsExceptionOnPgConnectFailure(): void {
		$hostname = 'my-host';
		$this->setExpectedException(
			Exception\DatabaseInitializationException::class,
			'Connection to host '.$hostname.' failed'
		);

		self::$functions
			->expects($this->once())
			->method('pg_connect')
			->willReturn(null);

		$this->configuration
			->expects($this->any())
			->method('getHost')
			->willReturn($hostname);

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
			->expects($this->once())
			->method('getHost')
			->willReturn($hostname);
		$this->configuration
			->expects($this->once())
			->method('getPort')
			->willReturn($port);
		$this->configuration
			->expects($this->once())
			->method('getName')
			->willReturn($name);
		$this->configuration
			->expects($this->once())
			->method('getUser')
			->willReturn($user);
		$this->configuration
			->expects($this->once())
			->method('getPassword')
			->willReturn($password);

		self::$functions
			->expects($this->once())
			->method('pg_connect')
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
			->willReturn($this->connection_resource);

		// UNSAFE
		$this->assertSame(
			$this->connection_resource,
			$this->database->getConnection()
		);
	}

	public function testQueryFailsButIncrementsQueryCount(): void {
		$this->createConnectionExpectation();
		$query_count_before = $this->database->getQueryCount();

		try {
			$this->database->query('NOTHING');
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
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, $query)
			->willReturn($result);

		$this->assertSame(
			$result,
			$this->database->query($query)
		);
	}

	public function testTransactionBeginSendsBeginQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, 'BEGIN')
			->willReturn(curl_init());

		$this->database->transactionBegin();
	}

	public function testTransactionCommitSendsCommitQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, 'COMMIT')
			->willReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testTransactionRollbackSendsRollbackQuery(): void {
		$this->createConnectionExpectation();

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, 'COMMIT')
			->willReturn(curl_init());

		$this->database->transactionCommit();
	}

	public function testGetNextResultReturnsResult(): void {
		$resource = curl_init();
		$result = ['key' => null];

		self::$functions
			->expects($this->once())
			->method('pg_fetch_assoc')
			->with($resource)
			->willReturn($result);

		$this->assertSame(
			$result,
			$this->database->getNextResult($resource)
		);
	}

	public function testQuoteReturnsEscapedString(): void {
		$this->createConnectionExpectation();

		$string = 'my-string';

		self::$functions
			->expects($this->once())
			->method('pg_escape_string')
			->with($this->connection_resource, $string)
			->willReturn($string);

		$this->assertSame(
			$string,
			$this->database->quote($string)
		);
	}

	public function testExistsReturnsTrueIfItemExists(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT EXISTS(meh)';
		$resource = curl_init();

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, $query)
			->willReturn($resource);

		self::$functions
			->expects($this->once())
			->method('pg_fetch_result')
			->with($resource, 0, 'exists')
			->willReturn('t');

		$this->assertTrue(
			$this->database->exists($query)
		);
	}

	public function testExistsReturnsFalseIfItemDoesNotExists(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT EXISTS(meh)';
		$resource = curl_init();

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, $query)
			->willReturn($resource);

		self::$functions
			->expects($this->once())
			->method('pg_fetch_result')
			->with($resource, 0, 'exists')
			->willReturn('f');

		$this->assertFalse(
			$this->database->exists($query)
		);
	}

	public function testCountReturnsNumberOfRows(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT COUNT(*)';
		$resource = curl_init();
		$count = 123;

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, $query)
			->willReturn($resource);

		self::$functions
			->expects($this->once())
			->method('pg_fetch_result')
			->with($resource, 0, 'count')
			->willReturn((string) $count);

		$this->assertSame(
			$count,
			$this->database->count($query)
		);
	}

	public function testGetLastInsertedIdReturnsId(): void {
		$this->createConnectionExpectation();

		$query = 'SELECT LASTVAL()';
		$resource = curl_init();
		$id = 666;

		self::$functions
			->expects($this->once())
			->method('pg_query')
			->with($this->connection_resource, $query)
			->willReturn($resource);

		self::$functions
			->expects($this->once())
			->method('pg_fetch_result')
			->with($resource, 0, 'lastval')
			->willReturn((int) $id);

		$this->assertSame(
			$id,
			$this->database->getLastInsertedId()
		);
	}

	private function createConnectionExpectation(): void {
		self::$functions
			->expects($this->once())
			->method('pg_connect')
			->willReturn($this->connection_resource);
	}
}
