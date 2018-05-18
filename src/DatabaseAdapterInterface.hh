<?hh // strict
namespace Usox\HaDb;

interface DatabaseAdapterInterface {

	public function query(string $query): \PDOStatement;

	public function getQueryCount(): int;

	public function transactionBegin(): bool;

	public function transactionCommit(): bool;

	public function transactionRollback(): bool;

	public function getNextResult(\PDOStatement $statement): ?array<string, ?string>;

	public function getConnection(): \PDO;

	public function quote(string $string): string;

	public function exists(string $query): bool;

	public function count(string $query): int;

	public function getLastInsertedId(string $sequence_name): int;

	public function emptyTable(string $table_name): void;
}
