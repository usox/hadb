<?hh // strict
namespace Usox\HaDb;

use function Facebook\FBExpect\expect;
use function Usox\HackMock\{mock, prospect};

class DatabaseAdapterTest extends \Usox\HackMock\HackMock {

    <<__LateInit>>
    private \PDO $pdo;

    <<__LateInit>>
    private DatabaseAdapter $adapter;

    <<__LateInit>>
    private DatabaseConfigInterface $config;

    <<__Override>>
    public async function beforeEachTestAsync(): Awaitable<void> {
        $this->config = mock(DatabaseConfigInterface::class);
        $this->pdo = mock(\PDO::class);

        $this->adapter = new DatabaseAdapter(
            function (DatabaseConfigInterface $config): \PDO {
                prospect($this->pdo, 'setAttribute')
                    ->with(\PDO::ATTR_ERRMODE, \PDO::ERRMODE_EXCEPTION)
                    ->once();
                return $this->pdo;
            },
            $this->config
        );
    }

    public function testGetConnectionReturnsConnection(): void {
        expect(
            $this->adapter->getConnection()
        )
        ->toBeSame($this->pdo);
    }

    public function testGetQueryCountIncrementsAfterQuery(): void {
        prospect($this->pdo, 'query')
			->with('foobar')
            ->once()
            ->andReturn(mock(\PDOStatement::class));

        $this->adapter->query('foobar');

        expect($this->adapter->getQueryCount())->toBeSame(1);
    }

    public function testGetConnectionThrowExceptionOnInitializationError(): void {
        $error_message = 'some-error-message';

        $adapter = new DatabaseAdapter(
            (DatabaseConfigInterface $config): \PDO ==> {
                throw new \PDOException($error_message);
            },
            mock(DatabaseConfigInterface::class)
        );
        expect(
            () ==> $adapter->getConnection()
        )
        ->toThrow(
            Exception\DatabaseInitializationException::class,
            $error_message
        );
    }
}
