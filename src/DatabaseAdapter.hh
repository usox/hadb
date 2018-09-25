<?hh // strict
namespace Usox\HaDb;

use namespace HH\Lib\Str;

final class DatabaseAdapter extends \PDO {

	private int $query_count = 0;

	public function __construct(
		private DatabaseConfigInterface $config
	): void {
		parent::__construct((string)$config);

		$this->setAttribute(\PDO::ATTR_ERRMODE, \PDO::ERRMODE_EXCEPTION);
	}

	public function getQueryCount(): int {
		return $this->query_count;
	}

	<<__Override>>
	public function query(string $query): \PDOStatement {
		$this->query_count++;

		return parent::query($query);
	}

	public function exists(string $query): bool {
		return $this->count($query) > 0;
	}

	public function count(string $query): int {
		return (int) $this->query($query)->fetchColumn();
	}

	public function getLastInsertedId(string $sequence_name): int {
		return (int) $this->lastInsertId($sequence_name);
	}

	public function emptyTable(string $table_name): void {
		$this->query(Str\format('TRUNCATE TABLE %s', $table_name));
	}
}
