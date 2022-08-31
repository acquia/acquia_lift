<?php

namespace Drupal\Tests\acquia_lift_publisher\Kernel\EventSubscriber\Publish;

use Acquia\ContentHubClient\CDF\CDFObject;
use Acquia\ContentHubClient\CDFAttribute;
use Acquia\ContentHubClient\CDFDocument;
use Acquia\ContentHubClient\ContentHubClient;
use Acquia\ContentHubClient\Settings;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\acquia_contenthub\Event\PrunePublishCdfEntitiesEvent;
use Drupal\acquia_contenthub_publisher\ContentHubPublisherEvents;
use Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent;
use Drupal\acquia_lift_publisher\Form\ContentPublishingForm;
use Drupal\Core\DependencyInjection\ContainerBuilder;
use Drupal\Core\Entity\Entity\EntityViewDisplay;
use Drupal\KernelTests\KernelTestBase;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;
use Drupal\Tests\RandomGeneratorTrait;
use PHPUnit\Framework\AssertionFailedError;
use Prophecy\Argument;
use Prophecy\PhpUnit\ProphecyTrait;

/**
 * Tests the Publish Only Rendered content.
 *
 * @group acquia_lift_publisher
 *
 * @coversDefaultClass \Drupal\acquia_lift_publisher\EventSubscriber\Publish\PublishOnlyRendered
 *
 * @requires module depcalc
 */
class PublishOnlyRenderedTest extends KernelTestBase {

  use ProphecyTrait;
  use ContentTypeCreationTrait;
  use NodeCreationTrait;
  use RandomGeneratorTrait;

  /**
   * Acquia Lift publisher settings.
   *
   * @var \Drupal\Core\Config\Config
   */
  private $publisherSettings;

  /**
   * The event dispatcher.
   *
   * @var \Symfony\Component\EventDispatcher\EventDispatcherInterface
   */
  private $eventDispatcher;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'acquia_contenthub',
    'acquia_contenthub_publisher',
    'acquia_lift',
    'acquia_lift_publisher',
    'depcalc',
    'field',
    'filter',
    'image',
    'node',
    'path_alias',
    'system',
    'taxonomy',
    'text',
    'user',
  ];

  /**
   * {@inheritdoc}
   */
  public function register(ContainerBuilder $container) {
    $client = $this->prophesize(ContentHubClient::class);
    $client->getEntities(Argument::type('array'))
      ->willReturn(new CDFDocument());

    $settings = $this->prophesize(Settings::class);
    $settings->getUuid()
      ->willReturn('93b95dc0-116a-477c-9a76-5944998295c1');

    $client_factory = $this->prophesize(ClientFactory::class);
    $client_factory->getClient()
      ->willReturn($client->reveal());
    $client_factory->getSettings()
      ->willReturn($settings->reveal());

    $container->set('acquia_contenthub.client.factory', $client_factory->reveal());

    parent::register($container);
  }

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();

    $this->installSchema('acquia_contenthub_publisher', ['acquia_contenthub_publisher_export_tracking']);

    $this->installEntitySchema('path_alias');
    $this->installEntitySchema('node');
    $this->installEntitySchema('entity_view_mode');
    $this->installEntitySchema('entity_view_display');
    $this->installEntitySchema('field_config');
    $this->installEntitySchema('user');
    $this->installEntitySchema('taxonomy_term');

    $this->installConfig([
      'acquia_lift_publisher',
      'image',
      'node',
      'filter',
      'system',
    ]);

    $this->createContentType([
      'type' => 'article',
      'name' => 'article',
    ]);

    $this->publisherSettings = $this->container
      ->get('config.factory')
      ->getEditable(ContentPublishingForm::CONFIG_NAME);
    $this->eventDispatcher = \Drupal::service('event_dispatcher');
  }

  /**
   * Tests default configuration content.
   */
  public function testDefaultConfiguration() {
    $pers_content_only = $this->publisherSettings->get(ContentPublishingForm::$pushSettingField);
    $this->assertTrue($pers_content_only, 'Personalized content push is active after installing module.');
  }

  /**
   * @covers ::onEnqueueCandidateEntity
   *
   * @throws \Exception
   */
  public function testOnEnqueueCandidateEntity() {
    $entity = $this->createNode([
      'title' => 'Test title',
      'type' => 'article',
    ]);

    // Set up entity view display for rendered entity creation.
    EntityViewDisplay::create([
      'id' => 'node.article',
      'targetEntityType' => 'node',
      'bundle' => 'article',
      'label' => 'Full',
      'mode' => 'full',
    ])->save();

    $operation = 'insert';

    $event = new ContentHubEntityEligibilityEvent($entity, $operation);
    $this->eventDispatcher->dispatch(ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY, $event);
    $this->assertFalse($event->getEligibility(), 'Entity not eligible due to the default publishing configuration');

    // Enable entity view display for the article, check if the entity is
    // eligible.
    $this->publisherSettings->set('view_modes', [
      'node' => [
        'article' => [
          'full' => 1,
        ],
      ],
    ]);
    $this->savePublisherSettings();

    $event = new ContentHubEntityEligibilityEvent($entity, $operation);
    $this->eventDispatcher->dispatch(ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY, $event);
    $this->assertTrue($event->getEligibility(), 'Entity is renderable, therefore eligible.');
  }

  /**
   * @covers ::onPrunePublishCdfEntities
   *
   * @throws \Exception
   */
  public function testOnPrunePublishCdfEntities() {
    $client = $this->container->get('acquia_contenthub.client.factory')->getClient();
    // Provide mock data.
    $cdf_mock = $this->getCdfMock();
    $document = new CDFDocument(...$cdf_mock->original);
    $unfiltered = $document->getEntities();

    $event = new PrunePublishCdfEntitiesEvent($client, $document, '175b8909-b873-4e8d-b054-896ba0293c46');
    $this->eventDispatcher->dispatch(AcquiaContentHubEvents::PRUNE_PUBLISH_CDF_ENTITIES, $event);
    $pruned = $event->getDocument()->getEntities();

    $this->assertGreaterThan($pruned, $unfiltered, 'Unnecessary content was removed from the CDF document');

    // Add the pruned CDF to the CDF test object, carry out assertions.
    $cdf_mock->pruned = $pruned;
    $this->assertContainsOnlyRequiredCdfs($cdf_mock);
  }

  /**
   * Saves publisher configuration.
   *
   * @throws \Exception
   */
  private function savePublisherSettings(): void {
    $this->publisherSettings->save();
    $this->container->get('acquia_lift_publisher.publishing_settings')
      ->setData($this->publisherSettings->getRawData());
  }

  /**
   * Asserts that the pruned CDF document only contains the required entities.
   *
   * @param object $cdf_mock
   *   The CDF test object to run the assertions against.
   */
  private function assertContainsOnlyRequiredCdfs(\stdClass $cdf_mock) {
    $orig_rendered_entity1 = $cdf_mock->rendered_entities[0]->getUuid();
    $orig_rendered_entity2 = $cdf_mock->rendered_entities[1]->getUuid();
    $pruned = $cdf_mock->pruned;

    $this->addToAssertionCount(1);
    if (!isset($pruned[$orig_rendered_entity1], $pruned[$orig_rendered_entity2])) {
      throw new AssertionFailedError('CDF document contains the rendered entities');
    }

    $expected = $cdf_mock->expected;
    $sorter = function (CDFObject $cdf1, CDFObject $cdf2) {
      return $cdf1->getUuid() <=> $cdf2->getUuid();
    };
    usort($expected, $sorter);
    usort($pruned, $sorter);

    $this->assertEquals($expected, array_values($pruned), 'The CDF document contains only the rendered entities, its source entity and the source entity tags.');
  }

  /**
   * Returns a testable CDF object.
   *
   * @return object
   *   The object containing the original CDF array, the rendered entity and
   *   the source entity.
   *
   * @throws \Exception
   */
  private function getCdfMock(): \stdClass {
    $uuid = \Drupal::getContainer()->get('uuid');
    $time = time();
    $cdfs = [];

    // Add random CDFs.
    $no_cdfs = 11;
    $origin = $uuid->generate();
    for ($i = 0; $i < $no_cdfs; $i++) {
      $cdfs[] = new CDFObject('drupal8_content_entity', $uuid->generate(), $time, $time, $origin);
    }

    $source_entity = new CDFObject('drupal8_content_entity', $uuid->generate(), $time, $time, $origin);
    $cdfs[] = $source_entity;

    // Add a few tags to source entity.
    $tags = array_slice($cdfs, 0, 3);
    $tag_uuids = [];
    /** @var \Acquia\ContentHubClient\CDF\CDFObject $tag */
    foreach ($tags as $tag) {
      $tag_uuids[] = $tag->getUuid();
    }
    $source_entity->addAttribute('tags', CDFAttribute::TYPE_ARRAY_REFERENCE, $tag_uuids);

    $rendered1 = new CDFObject('rendered_entity', $uuid->generate(), $time, $time, $origin);
    $rendered1->addAttribute('source_entity', CDFAttribute::TYPE_STRING, $source_entity->getUuid());
    $rendered2 = new CDFObject('rendered_entity', $uuid->generate(), $time, $time, $origin);
    $rendered2->addAttribute('source_entity', CDFAttribute::TYPE_STRING, $source_entity->getUuid());
    $cdfs[] = $rendered1;
    $cdfs[] = $rendered2;

    // Construct an easily testable object containing the original CDF array,
    // the rendered entity, and its source entity.
    $cdf_mock = new \stdClass();
    $cdf_mock->original = $cdfs;
    $cdf_mock->rendered_entities = [$rendered1, $rendered2];
    $cdf_mock->expected = array_merge([$source_entity], $tags, $cdf_mock->rendered_entities);

    return $cdf_mock;
  }

}
