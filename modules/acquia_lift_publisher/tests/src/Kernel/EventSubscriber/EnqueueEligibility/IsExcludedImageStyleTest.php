<?php

namespace Drupal\Tests\acquia_lift_publisher\Kernel\EventSubscriber\EnqueueEligibility;

use Drupal\acquia_contenthub_publisher\ContentHubPublisherEvents;
use Drupal\acquia_contenthub_publisher\Event\ContentHubEntityEligibilityEvent;
use Drupal\KernelTests\KernelTestBase;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;
use Drupal\Tests\taxonomy\Traits\TaxonomyTestTrait;

/**
 * Tests if ImageStyle is excluded.
 *
 * @group acquia_lift_publisher
 *
 * @package Drupal\Tests\acquia_lift_publisher\Kernel\EventSubscriber\EnqueueEligibility
 *
 * @covers \Drupal\acquia_lift_publisher\EventSubscriber\EnqueueEligibility\IsExcludedImageStyle
 *
 * @requires module depcalc
 */
class IsExcludedImageStyleTest extends KernelTestBase {

  use ContentTypeCreationTrait;
  use NodeCreationTrait;
  use TaxonomyTestTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'field',
    'filter',
    'node',
    'image',
    'text',
    'user',
    'system',
    'taxonomy',
    'depcalc',
    'acquia_contenthub',
    'acquia_contenthub_publisher',
    'acquia_lift_publisher',
  ];

  /**
   * Lift Registry.
   *
   * @var array
   */
  protected static $registry = [];

  /**
   * Event dispatcher.
   *
   * @var \Symfony\Component\EventDispatcher\EventDispatcher
   */
  protected $eventDispatcher;

  /**
   * Image style storage handler.
   *
   * @var \Drupal\image\ImageStyleStorage
   */
  private $imageStyleStorage;

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();

    $this->installSchema('acquia_contenthub_publisher', ['acquia_contenthub_publisher_export_tracking']);

    $this->installConfig([
      'filter',
      'node',
      'system',
      'image',
      'acquia_lift_publisher',
    ]);
    $this->installSchema('system', ['key_value_expire', 'sequences']);
    $this->installSchema('node', ['node_access']);
    $this->installEntitySchema('node');
    $this->installEntitySchema('user');
    $this->installEntitySchema('taxonomy_term');

    $this->eventDispatcher = \Drupal::service('event_dispatcher');

    try {
      $this->imageStyleStorage = \Drupal::entityTypeManager()
        ->getStorage('image_style');
      $this->prepareContent();
    }
    catch (\Exception $exception) {
      $this->markTestIncomplete($exception->getMessage());
    }
  }

  /**
   * Tests entity eligibility.
   *
   * @param string $operation
   *   Entity operation.
   * @param string $index
   *   Item's index in registry.
   * @param bool $expected
   *   Expected result.
   *
   * @dataProvider onEnqueueCandidateEntityDataProvider
   */
  public function testOnEnqueueCandidateEntity(string $operation, string $index, bool $expected) {
    if (empty(self::$registry[$index])) {
      $this->markTestIncomplete(sprintf('Specified item %s not found in registry', $index));
    }
    $entity = self::$registry[$index];

    $event = new ContentHubEntityEligibilityEvent($entity, $operation);
    $this->eventDispatcher->dispatch(ContentHubPublisherEvents::ENQUEUE_CANDIDATE_ENTITY, $event);

    $message = sprintf('Entity with index %s has wrong expected eligibility', $index);
    $this->assertEquals($event->getEligibility(), $expected, $message);
  }

  /**
   * Prepare test content.
   *
   * @throws \Exception
   */
  protected function prepareContent() {
    $this->createContentType(['type' => 'page']);

    /** @var \Drupal\taxonomy\Entity\Vocabulary $vocabulary */
    $vocabulary = $this->createVocabulary();
    self::$registry = [
      'node' => $this->createNode(),
      'term' => $this->createTerm($vocabulary),
      'eligible_image_style' => $this->imageStyleStorage->load('large'),
      'not_eligible_image_style' => $this->imageStyleStorage->load('acquia_lift_publisher_preview_image'),
    ];
  }

  /**
   * Data provider for testOnEnqueueCandidateEntity.
   */
  public function onEnqueueCandidateEntityDataProvider() {
    yield ['insert', 'node', TRUE];
    yield ['insert', 'term', TRUE];
    yield ['insert', 'eligible_image_style', TRUE];
    yield ['insert', 'not_eligible_image_style', FALSE];
    yield ['update', 'node', TRUE];
    yield ['update', 'term', TRUE];
    yield ['update', 'not_eligible_image_style', FALSE];
    yield ['update', 'eligible_image_style', TRUE];
  }

}
