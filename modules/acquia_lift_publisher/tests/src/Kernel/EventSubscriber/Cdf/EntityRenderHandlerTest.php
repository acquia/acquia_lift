<?php

namespace Drupal\Tests\acquia_lift_publisher\Kernel\EventSubscriber;

use Acquia\ContentHubClient\CDFDocument;
use Acquia\ContentHubClient\ContentHubClient;
use Acquia\ContentHubClient\Settings;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\acquia_contenthub\Event\CreateCdfEntityEvent;
use Drupal\block_content\BlockContentInterface;
use Drupal\block_content\Entity\BlockContent;
use Drupal\block_content\Entity\BlockContentType;
use Drupal\Core\Entity\EntityInterface;
use Drupal\KernelTests\KernelTestBase;
use Drupal\language\Entity\ConfigurableLanguage;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\RandomGeneratorTrait;
use Drupal\Tests\user\Traits\UserCreationTrait;
use Prophecy\Argument;

/**
 * Class EntityRenderHandlerTest.
 *
 * @coversDefaultClass \Drupal\acquia_lift_publisher\EventSubscriber\Cdf\EntityRenderHandler
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Cdf
 */
class EntityRenderHandlerTest extends KernelTestBase {

  use ContentTypeCreationTrait;
  use RandomGeneratorTrait;
  use UserCreationTrait;

  /**
   * The block type used in tests.
   *
   * @var \Drupal\Core\Entity\EntityInterface
   */
  protected $blockType;

  /**
   * {@inheritdoc}
   */
  protected $strictConfigSchema = FALSE;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'acquia_contenthub',
    'acquia_lift_publisher',
    'block',
    'block_content',
    'depcalc',
    'field',
    'filter',
    'language',
    'content_translation',
    'node',
    'system',
    'text',
    'user',
  ];

  /**
   * {@inheritdoc}
   *
   * @throws \ReflectionException
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  protected function setUp() {
    parent::setUp();

    $this->installEntitySchema('block_content');
    $this->installEntitySchema('block');
    $this->installEntitySchema('filter_format');
    $this->installEntitySchema('node');
    $this->installEntitySchema('user');
    $this->installSchema('system', 'sequences');
    $this->installConfig(['node', 'block_content']);

    $this->blockType = BlockContentType::create([
      'id' => $this->randomMachineName(),
      'label' => $this->randomString(),
    ]);
    $this->blockType->save();

    $document = $this->prophesize(CDFDocument::class);

    $client = $this->prophesize(ContentHubClient::class);
    $client->getEntities(Argument::type('array'))
      ->willReturn($document->reveal());

    $settings = new Settings(NULL, NULL, NULL, NULL, NULL);

    $client_factory = $this->prophesize(ClientFactory::class);
    $client_factory
      ->getClient()
      ->willReturn($client->reveal());
    $client_factory->getSettings()
      ->willReturn($settings);

    $this->container->set('acquia_contenthub.client.factory', $client_factory->reveal());
  }

  /**
   * @covers ::onCreateCdf
   *
   * @throws \Exception
   */
  public function testOnCreateCdfConfigEntity() {
    $config_entity = $this->createContentType([
      'id' => 'test_content_type',
      'name' => 'Test content type',
    ]);

    $this->enableViewModeExportFor($config_entity);
    $event = $this->dispatchWith($config_entity, []);
    $this->assertCdfNotHasRenderedEntity($event->getCdfList());
  }

  /**
   * @covers ::onCreateCdf
   *
   * @throws \Drupal\Core\Entity\EntityStorageException
   * @throws \Exception
   */
  public function testOnCreateCdfBlockContent() {
    ConfigurableLanguage::create([
      'id' => 'hu',
      'label' => 'Hungarian',
    ])->save();

    $block = $this->createBlockContent();
    $block->addTranslation('hu', [
      'info' => $this->randomString() . '- HU',
    ]);

    $this->enableViewModeExportFor($block);

    $event = $this->dispatchWith($block, []);
    $cdfs = $event->getCdfList();
    $this->assertCdfHasRenderedEntity($cdfs);
    // Assert 2 after the translation and the default language.
    $this->assertRenderedEntityCount($cdfs, 2);
  }

  /**
   * Dispatches the event in hand with an arbitrary input parameters.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The entity to use in the event.
   * @param array $dependencies
   *   The dependency stack.
   *
   * @return \Drupal\acquia_contenthub\Event\CreateCdfEntityEvent
   *   The altered event.
   *
   * @throws \Exception
   */
  protected function dispatchWith(EntityInterface $entity, array $dependencies): CreateCdfEntityEvent {
    $event = new CreateCdfEntityEvent($entity, $dependencies);
    $this->container->get('event_dispatcher')->dispatch(AcquiaContentHubEvents::CREATE_CDF_OBJECT, $event);
    return $event;
  }

  /**
   * Asserts that cdf list has a rendered entity.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   */
  protected function assertCdfNotHasRenderedEntity(array $cdfs): void {
    $entities = $this->getRenderedEntityCount($cdfs);
    $this->assertEqual($entities, 0, 'Cdf list does not contain rendered entit');
  }

  /**
   * Asserts that cdf list does not have a rendered entity.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   */
  protected function assertCdfHasRenderedEntity(array $cdfs): void {
    $entities = $this->getRenderedEntityCount($cdfs);
    $this->assertLessThan($entities, 0, 'Cdf list does contain rendered entities.');
  }

  /**
   * Asserts that cdf list has the desired number of rendered entities.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   *   The cdf list to test.
   * @param int $count
   *   The desired number of rendered entities.
   */
  public function assertRenderedEntityCount(array $cdfs, int $count): void {
    $entities = $this->getRenderedEntityCount($cdfs);
    $this->assertEqual($entities, $count, 'All entities were rendered.');
  }

  /**
   * Returns the number of rendered entities from the cdf list.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   *   The cdf list.
   *
   * @return int
   *   The number of rendered entities.
   */
  protected function getRenderedEntityCount(array $cdfs): int {
    $this->addToAssertionCount(1);
    $rendered_count = 0;
    foreach ($cdfs as $cdf) {
      if ($cdf->getType() === 'rendered_entity') {
        $rendered_count++;
      }
    }

    return $rendered_count;
  }

  /**
   * Enables view mode for certain entity rendered with the provided role.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The entity to enable export for.
   * @param string $render_role
   *   The user role to render the entity with.
   *
   * @throws \Exception
   */
  protected function enableViewModeExportFor(EntityInterface $entity, string $render_role = 'anonymous'): void {
    $this->container->get('config.factory')
      ->getEditable('acquia_lift_publisher.entity_config')
      ->set("view_modes.{$entity->getEntityTypeId()}.{$entity->bundle()}", ['full' => 1])
      ->set('render_role', $render_role)
      ->save();
  }

  /**
   * Creates a random block content.
   *
   * @return \Drupal\block_content\BlockContentInterface
   *   The block content.
   *
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  protected function createBlockContent(): BlockContentInterface {
    /** @var \Drupal\block_content\BlockContentInterface $block_content */
    $block_content = BlockContent::create([
      'info' => $this->randomString(),
      'type' => $this->blockType->id(),
      'reusable' => FALSE,
    ]);
    $block_content->save();

    return $block_content;
  }

}
