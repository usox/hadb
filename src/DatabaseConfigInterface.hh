<?hh // strict
namespace Usox\HaDb;

interface DatabaseConfigInterface {

	public function getHost(): string;

	public function getPort(): int;

	public function getName(): string;

	public function getUser(): string;

	public function getPassword(): string;
}
