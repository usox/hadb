<?hh // strict
namespace Usox\HaDb;

final class DatabaseMySql implements DatabaseInterface {

	private ?resource $connection;

	private int $query_count = 0;

	public function __construct(private DatabaseConfigInterface $config): void {
	}

	public function getConnection(): resource {
		if ($this->connection === null) {
			$this->connection = mysql_connect(
				sprintf('%s:%d', $this->config->getHost(), $this->config->getPort()),
				$this->config->getUser(),
				$this->config->getPassword()
			);
			if (false === mysql_select_db($this->config->getName(), $this->connection)) {
				throw new Exception\DatabaseInitializationException(
					sprintf('Database %s does not exist', $this->config->getName())
				);
			}
		}
		if (is_resource($this->connection)) {
			return $this->connection;
		}
		throw new Exception\DatabaseInitializationException(
			sprintf('Connection to host %s failed', $this->config->getHost())
		);
	}

	public function getQueryCount(): int {
		return $this->query_count;
	}

	public function query(string $query): resource {
		$this->query_count++;

		$result = mysql_query($query, $this->getConnection());
		if (is_resource($result)) {
			return $result;
		}
		throw new Exception\QueryFailedException(
			sprintf('Query failed: %s', $query)
		);
	}

	public function transactionBegin(): void {
		$this->query('START TRANSACTION');
	}

	public function transactionCommit(): void {
		$this->query('COMMIT');
	}

	public function transactionRollback(): void {
		$this->query('ROLLBACK');
	}

	public function getNextResult(resource $resource): ?array<string, ?string> {
		$result = mysql_fetch_assoc($resource);
		if ($result === false) {
			return null;
		}
		return $result;
	}

	public function quote(string $string): string {
		return mysql_real_escape_string($string, $this->getConnection());
	}

	public function exists(string $query): bool {
		return mysql_num_rows($this->query($query)) > 0;
	}   

	public function count(string $query): int {
		return (int) mysql_result($this->query($query), 0, 'count');
	}

	public function getLastInsertedId(): int {
		return (int) mysql_insert_id($this->getConnection());
	}
}
