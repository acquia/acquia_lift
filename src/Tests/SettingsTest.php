<?php

namespace Drupal\acquia_lift\Tests;

use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\simpletest\WebTestBase;

/**
 * Test Acquia Lift Settings.
 *
 * @group Acquia Lift
 */
class SettingsTest extends WebTestBase {

  use SettingsDataTrait;
  use FixturesDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['node', 'taxonomy', 'acquia_lift'];

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $permissions = [
      'access administration pages',
      'administer acquia lift',
      'administer content types',
      'administer modules',
      'administer site configuration',
    ];

    // Create article content type.
    $this->drupalCreateContentType(['type' => 'article', 'name' => 'Article']);

    // Create a Node.
    $this->drupalCreateNode([
      'nid' => 90210,
      'type' => 'article',
      'body' => [['value' => $this->randomMachineName(32), 'format' => 'full_html']],
    ]);

    // Create two vocabularies.
    $vocabulary1 = $this->createVocabulary();
    $vocabulary2 = $this->createVocabulary();

    $term_v1_t1 = $this->createTerm($vocabulary1);
    $term_v1_t2 = $this->createTerm($vocabulary1);
    $term_v2_t1 = $this->createTerm($vocabulary2);
    $term_v2_t2 = $this->createTerm($vocabulary2);
    $term_v2_t3 = $this->createTerm($vocabulary2);

    $field_country = $this->createFieldWithStorage('field_country', 'node', 'article', [$vocabulary1->id() => $vocabulary1->id()], ['target_type' => 'taxonomy_term'], 'entity_reference');
    $field_people = $this->createFieldWithStorage('field_people', 'node', 'article', [$vocabulary2->id() => $vocabulary2->id()], ['target_type' => 'taxonomy_term'], 'entity_reference');
    $field_tags = $this->createFieldWithStorage('field_tags', 'node', 'article', [$vocabulary2->id() => $vocabulary2->id()], ['target_type' => 'taxonomy_term'], 'entity_reference');

    // User to set up acquia_lift.
    $this->admin_user = $this->drupalCreateUser($permissions);
    $this->drupalLogin($this->admin_user);
  }

  public function testConfigurationLinks() {
    // Check if Configure link is available on 'Extend' page.
    // Requires 'administer modules' permission.
    $this->drupalGet('admin/modules');
    $this->assertRaw('admin/config/services/acquia-lift', '[testConfigurationLinks]: Configure link from Extend page to Acquia Lift Settings page exists.');

    // Check if Configure link is available on 'Status Reports' page. NOTE: Link is only shown without a configured Acquia Lift credential.
    // Requires 'administer site configuration' permission.
    $this->drupalGet('admin/reports/status');
    $this->assertRaw('admin/config/services/acquia-lift', '[testConfigurationLinks]: Configure link from Status Reports page to Acquia Lift Settings page exists.');
  }

  public function testAdminSettingsForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/services/acquia-lift');
    $this->assertRaw(t('Acquia Lift Settings'), '[testAdminSettingsForm]: Settings page displayed.');

    // Get all the valid settings, and massage them into form $edit array.
    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $field_mappings_settings = $this->getValidFieldMappingsSettings();
    $udf_person_settings = $this->getValidUdfPersonMappingsFormData();
    $udf_touch_settings = $this->getValidUdfTouchMappingsFormData();
    $udf_event_settings = $this->getValidUdfEventMappingsFormData();
    $visibility_settings = $this->getValidVisibilitySettings();
    $advanced_settings = $this->getValidAdvancedSettings();

    $edit =[];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');
    $edit += $this->convertToPostFormSettings($identity_settings, 'identity');
    $edit += $this->convertToPostFormSettings($field_mappings_settings, 'field_mappings');
    $edit += $this->convertToPostFormSettings($udf_person_settings, 'udf_person_mappings');
    $edit += $this->convertToPostFormSettings($udf_touch_settings, 'udf_touch_mappings');
    $edit += $this->convertToPostFormSettings($udf_event_settings, 'udf_event_mappings');
    $edit += $this->convertToPostFormSettings($visibility_settings, 'visibility');
    $edit += $this->convertToPostFormSettings($advanced_settings, 'advanced');
    $edit_settings_count = count($edit);
    $expect_settings_count = 19;

    // Post the edits.
    $this->drupalPostForm('admin/config/services/acquia-lift', $edit, new TranslatableMarkup('Save configuration'));

    // Assert error messages are set for required fields and unreachable URLs.
    $this->assertText(t('The Acquia Lift module requires a valid Account ID, Site ID, and Assets URL to complete activation.'));
    $this->assertText(t('Acquia Lift module could not reach the specified Assets URL.'));
    $this->assertText(t('Acquia Lift module could not reach the specified Decision API URL.'));

    // Assert all other fields. Also count the asserted fields to make sure all are asserted.
    foreach ($edit as $name => $value) {
      $this->assertFieldByName($name, $value, format_string('"@name" setting was saved into DB.', ['@name' => $name]));
    }
    $this->assertEqual($expect_settings_count, $edit_settings_count, 'The exact numbers of settings that were asserted should be ' . $expect_settings_count . '.');

    // Assert metatags are loaded in the header.
    $this->drupalGet('node/90210');
  }

  public function testMetatagsAndScriptTag() {
    $this->setValidSettings();

    // Assert metatags are loaded in the header.
    $this->drupalGet('node/90210', ['query' => ['my_identity_parameter' => 'an_identity']]);
    $this->assertRaw('an_identity', '[testMetatagsAndScriptTag]: identity metatag value is loaded on the node page.');
    $this->assertRaw('acquia_lift:page_type', '[testMetatagsAndScriptTag]: page_type metatag is loaded on the node page.');
    $this->assertRaw('node page', '[testMetatagsAndScriptTag]: page_type metatag value is loaded on the node page.');
    $this->assertRaw('acquia_lift:account_id', '[testMetatagsAndScriptTag]: account_id metatag is loaded on the node page.');
    $this->assertRaw('AccountId1', '[testMetatagsAndScriptTag]: account_id metatag value is loaded on the node page.');
    $this->assertRaw('acquia_lift:bootstrapMode', '[testMetatagsAndScriptTag]: bootstrap mode metatag is loaded on the node page.');
    $this->assertRaw('manual', '[testMetatagsAndScriptTag]: bootstrap mode metatag value is loaded on the node page.');
    $this->assertRaw('acquia_lift:contentReplacementMode', '[testMetatagsAndScriptTag]: content replacement mode metatag is loaded on the node page.');
    $this->assertRaw('customized', '[testMetatagsAndScriptTag]: content replacement mode metatag value is loaded on the node page.');

    // Assert Lift JavaScript tag is async-loaded on the page.
    $this->assertRaw('AssetsUrl1', '[testMetatagsAndScriptTag]: With valid settings, Lift\'s JavaScript is loaded on the home page.');
    $this->assertRaw('async', '[testMetatagsAndScriptTag]: With valid settings, Lift\'s JavaScript is async-loaded on the home page.');
  }
}
