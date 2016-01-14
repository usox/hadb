<?hh // strict
namespace Usox\HaDb;

interface DatabaseInterface {

	public function query(string $query): resource;

	public function getQueryCount(): int;

	public function transactionBegin(): void;

	public function transactionCommit(): void;

	public function transactionRollback(): void;

	public function getNextResult(resource $pgsql_resource): ?array<string, ?string>;

	public function getConnection(): resource;

	public function quote(string $string): string;

	public function exists(string $query): bool;

	public function count(string $query): int;

	public function getLastInsertedId(): int;
}
