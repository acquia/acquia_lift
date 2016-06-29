<?php

namespace Drupal\acquia_lift\Tests;

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
    $this->assertRaw('admin/config/content/acquia-lift', '[testConfigurationLinks]: Configure link from Extend page to Acquia Lift Settings page exists.');

    // Check if Configure link is available on 'Status Reports' page. NOTE: Link is only shown without a configured Acquia Lift credential.
    // Requires 'administer site configuration' permission.
    $this->drupalGet('admin/reports/status');
    $this->assertRaw('admin/config/content/acquia-lift', '[testConfigurationLinks]: Configure link from Status Reports page to Acquia Lift Settings page exists.');
  }

  public function testAdminSettingsForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift');
    $this->assertRaw(t('Acquia Lift settings'), '[testAdminSettingsForm]: Settings page displayed.');

    // Get all the valid settings, and massage them into form $edit array.
    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $field_mappings_settings = $this->getValidFieldMappingsSettings();
    $visibility_settings = $this->getValidVisibilitySettings();

    $edit =[];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');
    $edit += $this->convertToPostFormSettings($identity_settings, 'identity');
    $edit += $this->convertToPostFormSettings($field_mappings_settings, 'field_mappings');
    $edit += $this->convertToPostFormSettings($visibility_settings, 'visibility');
    $edit_settings_count = count($edit);
    $expect_settings_count = 12;

    // Post the edits and assert that options are saved.
    $this->drupalPostForm('admin/config/content/acquia-lift', $edit, t('Save configuration'));
    $this->assertText(t('The configuration options have been saved.'));
    $this->assertNoRaw('acquia_lift.js', '[testJavaScriptAndDrupalSettings]: acquia_lift.js is not loaded on the page, as visibility.path_patterns says should not attach.');

    // Assert all other fields. Also count the asserted fields to make sure all are asserted.
    foreach ($edit as $name => $value) {
      $this->assertFieldByName($name, $value, format_string('"@name" setting was saved into DB.', array('@name' => $name)));
    }
    $this->assertEqual($expect_settings_count, $edit_settings_count, 'The exact numbers of settings that were asserted should be ' . $expect_settings_count . '.');

    // Assert the Thumbnail URL shortcut links exist on the page.
    $this->assertRaw('admin/structure/types/manage/article#edit-acquia-lift', '[testAdminSettingsForm]: Thumbnail URL shortcut links exist on the page.');

    // Assert the node type thumbnail form is actually loaded at the node type configuration page.
    $this->drupalGet('admin/structure/types/manage/article');
    $this->assertText(t('Acquia Lift'));
  }

  public function testJavaScriptAndDrupalSettings() {
    $this->setValidSettings();

    // Assert drupalSettings with identity query parameters on a non-node page.
    $this->drupalGet('page-not-found', ['query' => ['my_identity_parameter' => 'an_identity']]);
    $drupalSettings = $this->getDrupalSettings();
    $this->assertRaw('acquia_lift.js', '[testJavaScriptAndDrupalSettings]: With valid settings, acquia_lift.js is loaded on the home page.');
    $this->assertEqual('account_name_1', $drupalSettings['acquia_lift']['credential']['account_name'], '[testJavaScriptAndDrupalSettings]: JavaScript settings contain credential.account_name.');
    $this->assertEqual('an_identity', $drupalSettings['acquia_lift']['identity']['identity'], '[testJavaScriptAndDrupalSettings]: JavaScript settings contain identity.identity.');

    // Assert drupalSettings with identity query parameters on a node page.
    $this->drupalGet('node/90210', ['query' => ['my_identity_parameter' => 'an_identity']]);
    $drupalSettings = $this->getDrupalSettings();
    $this->assertRaw('acquia_lift.js', '[testJavaScriptAndDrupalSettings]: With valid settings, acquia_lift.js is loaded on the node page.');
    $this->assertEqual('account_name_1', $drupalSettings['acquia_lift']['credential']['account_name'], '[testJavaScriptAndDrupalSettings]: JavaScript settings contain credential.account_name.');
    $this->assertEqual('an_identity', $drupalSettings['acquia_lift']['identity']['identity'], '[testJavaScriptAndDrupalSettings]: JavaScript settings contain identity.identity.');

    // Assert metatags are loaded in the header.
    $this->assertRaw('acquia_lift:page_type', '[testJavaScriptAndDrupalSettings]: page_type metatag is loaded on the node page.');
    $this->assertRaw('node page', '[testJavaScriptAndDrupalSettings]: page_type metatag value is loaded on the node page.');
    $this->assertRaw('acquia_lift:account_id', '[testJavaScriptAndDrupalSettings]: account_id metatag is loaded on the node page.');
    $this->assertRaw('account_name_1', '[testJavaScriptAndDrupalSettings]: account_id metatag value is loaded on the node page.');
  }
}
