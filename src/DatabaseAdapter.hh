<?hh // strict
namespace Usox\HaDb;

final class DatabaseAdapter implements DatabaseAdapterInterface {

	private ?\PDO $connection;

	private int $query_count = 0;

	public function __construct(
		private (function(DatabaseConfigInterface): \PDO) $initialization,
		private DatabaseConfigInterface $config
	): void {
	}

	public function getConnection(): \PDO {
		if ($this->connection === null) {
			try {
				$this->connection = call_user_func($this->initialization, $this->config);
				$this->connection->setAttribute(\PDO::ATTR_ERRMODE, \PDO::ERRMODE_EXCEPTION);
			} catch (\PDOException $e) {
				throw new Exception\DatabaseInitializationException(
					$e->getMessage()
				);
			}

			if ($this->connection === null) {
				throw new Exception\DatabaseInitializationException('Database initialization failed');
			}
		}

		return $this->connection;
	}

	public function getQueryCount(): int {
		return $this->query_count;
	}

	public function query(string $query): \PDOStatement {
		$this->query_count++;

		try {
			return $this->getConnection()->query($query);
		} catch (\PDOException $e) {
			throw new Exception\QueryFailedException(
				$e->getMessage()
			);
		}
	}

	public function transactionBegin(): bool {
		return $this->getConnection()->beginTransaction();
	}

	public function transactionCommit(): bool {
		return $this->getConnection()->commit();
	}

	public function transactionRollback(): bool {
		return $this->getConnection()->rollBack();
	}

	public function getNextResult(\PDOStatement $statement): ?array<string, ?string> {
		$result = $statement->fetch(\PDO::FETCH_ASSOC);
		if ($result === false) {
			return null;
		}
		return $result;
	}

	public function quote(string $string): string {
		return $this->getConnection()->quote($string);
	}

	public function exists(string $query): bool {
		return $this->count($query) > 0;
	}

	public function count(string $query): int {
		return (int) $this
			->query($query)
			->fetchColumn();
	}

	public function getLastInsertedId(string $sequence_name): int {
		return (int) $this
			->getConnection()
			->lastInsertId($sequence_name);
	}

	public function emptyTable(string $table_name): void {
		$this->query(\sprintf('TRUNCATE TABLE %s', $table_name));
	}

	public static function factory(DatabaseConfigInterface $config): DatabaseAdapter {
		return new DatabaseAdapter(
			(DatabaseConfigInterface $config): \PDO ==>  {
				return new \PDO((string) $config);
			},
			$config
		);
	}
}
