<?hh // strict
namespace Usox\HaDb;

final class DatabasePostgres implements DatabaseInterface {

	private ?resource $connection;

	private int $query_count = 0;

	public function __construct(private DatabaseConfigInterface $config): void {
	}

	public function getConnection(): resource {
		if ($this->connection === null) {
			$this->connection = pg_connect(
				sprintf(
					'host=%s port=%d dbname=%s user=%s password=%s',
					$this->config->getHost(),
					$this->config->getPort(),
					$this->config->getName(),
					$this->config->getUser(),
					$this->config->getPassword()
				)
			);
		}
		if ($this->connection === null) {
			throw new Exception\DatabaseInitializationException(
				sprintf('Connection to host %s failed', $this->config->getHost())
			);
		}
		return $this->connection;
	}

	public function getQueryCount(): int {
		return $this->query_count;
	}

	public function query(string $query): resource {
		$this->query_count++;

		$result = pg_query($this->getConnection(), $query);	
		if ($result === null) {
			throw new Exception\QueryFailedException(
				sprintf('Query failed: %s',$query)
			);
		}
		return $result;
	}

	public function transactionBegin(): void {
		$this->query('BEGIN');
	}

	public function transactionCommit(): void {
		$this->query('COMMIT');
	}

	public function transactionRollback(): void {
		$this->query('ROLLBACK');
	}

	public function getNextResult(resource $pgsql_resource): ?array<string, ?string> {
		return pg_fetch_assoc($pgsql_resource);
	}

	public function quote(string $string): string {
		return pg_escape_string($this->getConnection(), $string);
	}

        public function exists(string $query): bool {
                return pg_fetch_result($this->query($query), 0, 'exists') === 't';
        }   

        public function count(string $query): int {
                return (int) pg_fetch_result($this->query($query), 0, 'count');
        }

	public function getLastInsertedId(): int {
		return (int) pg_fetch_result($this->query('SELECT LASTVAL()'), 0, 'lastval');
	}
}
