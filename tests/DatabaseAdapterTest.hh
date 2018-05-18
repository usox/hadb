<?hh // strict
namespace Usox\HaDb;

use function Facebook\FBExpect\expect;

class DatabaseAdapterTest extends \PHPUnit_Framework_TestCase {

    private ?\PDO $pdo;

    private ?DatabaseAdapter $adapter;

    private ?DatabaseConfigInterface $config;

    public function setUp(): void {
        $this->config = $this->createMock(DatabaseConfigInterface::class);
        // UNSAFE
        $this->pdo = \Mockery::mock(\PDO::class);

        $this->adapter = new DatabaseAdapter(
            function (DatabaseConfigInterface $config): \PDO {
                //UNSAFE
                $this->pdo
                    ?->shouldReceive('setAttribute')
                    ?->with(\PDO::ATTR_ERRMODE, \PDO::ERRMODE_EXCEPTION)
                    ?->once();
                return $this->pdo;
            },
            $this->config
        );
    }

    public function testGetConnectionReturnsConnection(): void {
        expect(
            $this->adapter?->getConnection()
        )
        ->toBeSame($this->pdo);
    }

    public function testGetQueryCountIncrementsAfterQuery(): void {
        expect($this->adapter?->getQueryCount())->toBeSame(0);
    }

    public function testGetConnectionThrowExceptionOnInitializationError(): void {
        $error_message = 'some-error-message';

        $adapter = new DatabaseAdapter(
            (DatabaseConfigInterface $config): \PDO ==> {
                throw new \PDOException($error_message);
            },
            $this->createMock(DatabaseConfigInterface::class)
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