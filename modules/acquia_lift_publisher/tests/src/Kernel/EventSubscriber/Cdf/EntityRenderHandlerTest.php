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
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Language\LanguageInterface;
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
    $this->installConfig(['node', 'block_content', 'user']);

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
      'info' => $this->randomString(),
    ]);

    $this->enableViewModeExportFor($block);

    $event = $this->dispatchWith($block, []);
    $cdfs = $this->getRenderedEntities($event->getCdfList());
    $this->assertCount(2, $cdfs, 'All entities were rendered.');
    $this->assertCdfAttributes($block, $cdfs);
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
   * Asserts that cdf list doesn't have a rendered entity.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   */
  protected function assertCdfNotHasRenderedEntity(array $cdfs): void {
    $entities = $this->getRenderedEntities($cdfs);
    $this->assertEqual(count($entities), 0, 'Cdf list does not contain rendered entity.');
  }

  /**
   * Asserts that entity related cdf list has the correct values and attributes.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $entity
   *   The entity to test.
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   *   The cdf list to compare.
   */
  protected function assertCdfAttributes(ContentEntityInterface $entity, array $cdfs): void {
    $original_languages = [];
    foreach ($entity->getTranslationLanguages() as $translation_language) {
      $original_languages[] = $translation_language->getId();
    }

    // The attributes to check in rendered entities.
    $cdf_languages = [];
    $source_entities = [];
    $contents = [];

    foreach ($cdfs as $cdf) {
      $language = $cdf->getAttribute('language');
      $this->assertNotNull($language, 'Entity translation has a corresponding cdf.');
      $language = $language->getValue()[LanguageInterface::LANGCODE_NOT_SPECIFIED];
      $cdf_languages[] = $language;

      $source_entities[] = $cdf->getAttribute('source_entity')
        ->getValue()[LanguageInterface::LANGCODE_NOT_SPECIFIED];

      $contents[$language] = $cdf->getAttribute('content')
        ->getValue()[LanguageInterface::LANGCODE_NOT_SPECIFIED];
    }

    // These entities must come from the same source.
    $entity_uuid = $entity->uuid();
    $this->assertTrue($source_entities[0] === $entity_uuid, 'Source uuid and original uuid match.');
    $this->assertTrue($source_entities[1] === $entity_uuid, 'Source uuid and original uuid match.');

    $this->assertEqual($cdf_languages, $original_languages, 'All the translations have been rendered.');

    // Check if the content are translation specific.
    foreach ($original_languages as $original_language) {
      $translation = $entity->getTranslation($original_language);
      $orig_label = $translation->label();
      $this->assertNotFalse(strpos(htmlspecialchars($orig_label), $contents[$original_language]), 'Cdf contains the translated content.');
    }
  }

  /**
   * Returns the rendered entities from the cdf list.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   *   The cdf list.
   *
   * @return \Acquia\ContentHubClient\CDF\CDFObject[]
   *   A set of rendered entities.
   */
  protected function getRenderedEntities(array $cdfs): array {
    $rendered_entities = [];
    foreach ($cdfs as $cdf) {
      if ($cdf->getType() === 'rendered_entity') {
        $rendered_entities[] = $cdf;
      }
    }

    return $rendered_entities;
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
  protected function enableViewModeExportFor(EntityInterface $entity, string $render_role = 'administrator'): void {
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
      'reusable' => TRUE,
    ]);
    $block_content->save();

    return $block_content;
  }

}
