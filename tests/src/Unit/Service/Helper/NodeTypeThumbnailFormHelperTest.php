<?php

namespace Drupal\Tests\acquia_lift\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\NodeTypeThumbnailFormHelper;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;
use Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal\ImageStyleOptions;

require_once(__DIR__ . '/../../Traits/SettingsDataTrait.php');
require_once(__DIR__ . '/../../Polyfill/Drupal.php');

/**
 * NodeTypeThumbnailFormHelper Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\NodeTypeThumbnailFormHelper
 * @group acquia_lift
 */
class NodeTypeThumbnailFormHelperTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $configFactory;

  /**
   * @var \Drupal\Core\Entity\EntityManagerInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $entityManager;

  /**
   * @var \Drupal\Core\Config\Config|\PHPUnit_Framework_MockObject_MockObject
   */
  private $settings;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->configFactory = $this->getMock('Drupal\Core\Config\ConfigFactoryInterface');
    $this->entityManager = $this->getMock('Drupal\Core\Entity\EntityManagerInterface');
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\Config')
      ->disableOriginalConstructor()
      ->getMock();

    $this->configFactory->expects($this->once())
      ->method('getEditable')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);
  }

  /**
   * Tests the getForm() method, no field for this entity.
   *
   * @covers ::getForm
   */
  public function testGetFormNoField() {
    $form_helper = new NodeTypeThumbnailFormHelper($this->configFactory, $this->entityManager);
    $this->entityManager->expects($this->once())
      ->method('getFieldDefinitions')
      ->with('node', 'article')
      ->willReturn([]);

    $form = $form_helper->getForm('article');
    $this->assertRegexp('/no image field/', $form['no_image_field']['#markup']);
  }

  /**
   * Tests the getForm() method, no style.
   *
   * @covers ::getForm
   */
  public function testGetFormWithNoStyle() {
    $form_helper = new NodeTypeThumbnailFormHelper($this->configFactory, $this->entityManager);
    $field_definitions = [
      'field_description' => $this->getFieldDefinition('description'),
      'field_image' => $this->getFieldDefinition('image'),
    ];
    $this->entityManager->expects($this->once())
      ->method('getFieldDefinitions')
      ->with('node', 'article')
      ->willReturn($field_definitions);

    $form = $form_helper->getForm('article');
    $this->assertRegexp('/no image style/', $form['no_image_styles']['#markup']);
  }

  /**
   * Tests the getForm() method, with an image field and with style.
   *
   * @covers ::getForm
   */
  public function testGetFormWithFieldAndStyle() {
    $form_helper = new NodeTypeThumbnailFormHelper($this->configFactory, $this->entityManager);
    $field_image_definition = $this->getFieldDefinition('image');
    $field_definitions = [
      'field_description' => $this->getFieldDefinition('description'),
      'field_image' => $field_image_definition,
      'field_entity_reference' => $this->getFieldDefinition('entity_reference'),
    ];
    $entity_reference_field_definitions = [
      'field_child_image' => $this->getFieldDefinition('image'),
      'field_image' => $field_image_definition, // This line is testing circular reference handling. See $processedFieldHashes.
    ];
    $entity_type_definition = $this->getMock('Drupal\Core\Entity\EntityTypeInterface');

    $this->entityManager->expects($this->at(0))
      ->method('getFieldDefinitions')
      ->with('node', 'article')
      ->willReturn($field_definitions);
    $this->settings->expects($this->once())
      ->method('get')
      ->with('thumbnail')
      ->willReturn($this->getValidThumbnailSettings());
    $this->entityManager->expects($this->at(1))
      ->method('getDefinition')
      ->with('entity_reference_setting')
      ->willReturn($entity_type_definition);
    $entity_type_definition->expects($this->once())
      ->method('isSubclassOf')
      ->with('\Drupal\Core\Entity\FieldableEntityInterface')
      ->willReturn(TRUE);
    $this->entityManager->expects($this->at(2))
      ->method('getFieldDefinitions')
      ->with('entity_reference_setting', 'entity_reference')
      ->willReturn($entity_reference_field_definitions);

    ImageStyleOptions::$return = ['medium' => 'Medium'];

    $form = $form_helper->getForm('article');

    $expected_field_options = [
      'field_image' => 'Image Label (field_image)',
      'field_entity_reference->field_child_image' => 'Entity_reference Label->Image Label (field_entity_reference->field_child_image)',
    ];
    $this->assertEquals('Acquia Lift', $form['#title']);
    $this->assertEquals($expected_field_options, $form['field']['#options']);
    $this->assertEquals('field_media->field_image', $form['field']['#default_value']);
    $this->assertEquals(['medium' => 'Medium'], $form['style']['#options']);
    $this->assertEquals('medium', $form['style']['#default_value']);
  }

  /**
   * Tests the saveSettings() method.
   *
   * @covers ::saveSettings
   */
  public function testSaveSettings() {
    $set_settings = ['article' => ['some_settings']];
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('thumbnail')
      ->willReturn($this->getValidThumbnailSettings());
    $this->settings->expects($this->at(1))
      ->method('set')
      ->with('thumbnail', $set_settings)
      ->willReturn($this->settings);
    $this->settings->expects($this->at(2))
      ->method('save');

    $form_helper = new NodeTypeThumbnailFormHelper($this->configFactory, $this->entityManager);
    $form_helper->saveSettings('article', ['some_settings']);
  }

  /**
   * Get FieldDefinition mock.
   *
   * @param string $type
   *   FieldDefinition type.
   */
  private function getFieldDefinition($type = 'other') {
    $field_definition = $this->getMock('Drupal\Core\Field\FieldDefinitionInterface');
    $field_definition->expects($this->at(0))
      ->method('getType')
      ->willReturn($type);
    $field_definition->expects($this->at(1))
      ->method('getSetting')
      ->with('target_type')
      ->willReturn($type . '_setting');
    $field_definition->expects($this->at(2))
      ->method('getLabel')
      ->willReturn(ucfirst($type) . ' Label');
    return $field_definition;
  }
}
