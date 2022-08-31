<?php

namespace Drupal\Tests\acquia_lift_publisher\Kernel\EventSubscriber\Cdf;

use Acquia\ContentHubClient\CDFDocument;
use Acquia\ContentHubClient\ContentHubClient;
use Acquia\ContentHubClient\Settings;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\acquia_contenthub\Event\CreateCdfEntityEvent;
use Drupal\acquia_lift_publisher\EventSubscriber\Cdf\EntityRenderHandler;
use Drupal\block_content\BlockContentInterface;
use Drupal\block_content\Entity\BlockContent;
use Drupal\block_content\Entity\BlockContentType;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Language\LanguageInterface;
use Drupal\file\Entity\File;
use Drupal\image\Entity\ImageStyle;
use Drupal\KernelTests\KernelTestBase;
use Drupal\language\Entity\ConfigurableLanguage;
use Drupal\Tests\image\Kernel\ImageFieldCreationTrait;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;
use Drupal\Tests\RandomGeneratorTrait;
use Drupal\Tests\TestFileCreationTrait;
use Drupal\Tests\user\Traits\UserCreationTrait;
use Prophecy\Argument;
use Prophecy\PhpUnit\ProphecyTrait;

/**
 * Tests EntityRenderHandler.
 *
 * @coversDefaultClass \Drupal\acquia_lift_publisher\EventSubscriber\Cdf\EntityRenderHandler
 *
 * @group acquia_lift_publisher
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Cdf
 */
class EntityRenderHandlerTest extends KernelTestBase {

  use ProphecyTrait;
  use ContentTypeCreationTrait;
  use RandomGeneratorTrait;
  use UserCreationTrait;
  use ImageFieldCreationTrait;
  use NodeCreationTrait;
  use TestFileCreationTrait;

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
    'acquia_contenthub_publisher',
    'acquia_lift',
    'acquia_lift_publisher',
    'block',
    'block_content',
    'depcalc',
    'field',
    'filter',
    'language',
    'content_translation',
    'node',
    'path_alias',
    'system',
    'text',
    'user',
    'file',
    'image',
  ];

  /**
   * {@inheritdoc}
   *
   * @throws \ReflectionException
   * @throws \Drupal\Core\Entity\EntityStorageException
   */
  protected function setUp(): void {
    parent::setUp();

    $this->installSchema('acquia_contenthub_publisher', ['acquia_contenthub_publisher_export_tracking']);
    $this->installEntitySchema('block_content');
    $this->installEntitySchema('block');
    $this->installEntitySchema('filter_format');
    $this->installEntitySchema('file');
    $this->installEntitySchema('node');
    $this->installEntitySchema('path_alias');
    $this->installEntitySchema('user');
    $this->installSchema('system', 'sequences');
    $this->installSchema('file', 'file_usage');
    $this->installConfig(['node', 'block_content', 'user', 'file', 'image', 'filter', 'acquia_lift_publisher']);

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
  public function testOnCreateCdfMetadataSetCorrectly() {
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
    $rendered_entities = $this->getRenderedEntities($event->getCdfList());
    $this->assertCdfListHasMetadata($rendered_entities);
  }

  /**
   * @covers ::onCreateCdf
   *
   * @throws \Exception
   */
  public function testImageAttributeIsSet() {
    $this->createContentType([
      'id' => 'article',
      'name' => 'Image article content type',
      'type' => 'article',
    ]);

    $this->createImageField('field_image_test', 'article', [], [], [], [], 'Image test on [site:name]');
    $image_files = $this->getTestFiles('image');
    $image = File::create((array) current($image_files));
    $image->save();

    $entity = $this->createNode([
      'type' => 'article',
      'title' => 'Title Test',
      'field_image_test' => [
        [
          'target_id' => $image->id(),
        ],
      ],
    ]);

    $this->enableViewModeExportFor($entity);
    $event = $this->dispatchWith($entity, []);
    $cdfs = $this->getRenderedEntities($event->getCdfList());

    $cdf = current($cdfs);
    $this->assertNotNull($cdf);

    // Assert that image url is correct.
    $this->assertEquals($cdf->getAttribute('preview_image')->getValue()['und'], ImageStyle::load('acquia_lift_publisher_preview_image')->buildUrl($image->getFileUri()), '');

    // Ensure that a node with an empty image field can get rendered (LEB-4401).
    // Create another node with no image.
    $entity = $this->createNode([
      'type' => 'article',
      'title' => 'Title test with no image',
    ]);

    $event = $this->dispatchWith($entity, []);
    $rendered_cdfs = $this->getRenderedEntities($event->getCdfList());
    $this->assertCount(1, $rendered_cdfs, 'Entity rendered.');

    $cdf = current($rendered_cdfs);
    // Check that title matches.
    $this->assertEquals($cdf->getAttribute('label')->getValue()['en'], 'Title test with no image');
    // Check that no image preview is present in CDF.
    $this->assertNull(
      $cdf->getAttribute('preview_image'),
      'No preview image in CDF'
    );
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
    // Start with a clean sheet. It is possible that the container's
    // content has been modified.
    $handler = new EntityRenderHandler(
      $this->container->get('account_switcher'),
      $this->container->get('acquia_lift_publisher.publishing_settings'),
      $this->container->get('renderer'),
      $this->container->get('entity_type.manager'),
      $this->container->get('plugin.manager.block'),
      $this->container->get('uuid'),
      $this->container->get('acquia_contenthub.client.factory'),
      $this->container->get('language.default'),
      $this->container->get('string_translation')
    );
    $this->container->set('acquia_lift.service.entity_render.cdf.handler', $handler);

    $event = new CreateCdfEntityEvent($entity, $dependencies);
    $this->container->get('event_dispatcher')->dispatch($event, AcquiaContentHubEvents::CREATE_CDF_OBJECT);
    return $event;
  }

  /**
   * Asserts that cdf list doesn't have a rendered entity.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject[] $cdfs
   *   Lift CDFs.
   */
  protected function assertCdfNotHasRenderedEntity(array $cdfs): void {
    $entities = $this->getRenderedEntities($cdfs);
    $this->assertEquals(count($entities), 0, 'Cdf list does not contain rendered entity.');
  }

  /**
   * Asserts that each cdf o a given list has metadata being set.
   *
   * @param array $cdf_list
   *   The cdf list.
   */
  protected function assertCdfListHasMetadata(array $cdf_list) {
    foreach ($cdf_list as $cdf) {
      $this->assertNotEmpty($cdf->getMetadata(), 'Metadata is not set');
    }
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

    $this->assertEquals($cdf_languages, $original_languages, 'All the translations have been rendered.');

    // Check if the content are translation specific.
    foreach ($original_languages as $original_language) {
      $translation = $entity->getTranslation($original_language);
      $orig_label = $translation->label();
      $this->assertNotFalse(strpos($contents[$original_language], htmlspecialchars($orig_label, ENT_QUOTES)), 'Cdf contains the translated content.');
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
  protected function enableViewModeExportFor(EntityInterface $entity, string $render_role = 'anonymous'): void {
    $config = $this->container->get('config.factory')
      ->getEditable('acquia_lift_publisher.entity_config');
    $config->set("view_modes.{$entity->getEntityTypeId()}.{$entity->bundle()}", ['full' => 1])
      ->set("view_modes.node.article.acquia_lift_preview_image", 'field_image_test')
      ->set('render_role', $render_role)
      ->save();

    $config = $this->container->get('config.factory')
      ->get('acquia_lift_publisher.entity_config');
    $this->container->set('acquia_lift_publisher.publishing_settings', $config);
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
