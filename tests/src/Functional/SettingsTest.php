<?php

namespace Drupal\Tests\acquia_lift\Functional;

use Drupal\Core\StringTranslation\TranslatableMarkup;
use Drupal\Tests\acquia_lift\Unit\Traits\FixturesDataTrait;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;
use Drupal\Tests\BrowserTestBase;

/**
 * Test Acquia Lift Settings.
 *
 * @group acquia_lift
 */
class SettingsTest extends BrowserTestBase {

  use SettingsDataTrait;
  use FixturesDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  protected static $modules = ['node', 'taxonomy', 'acquia_lift'];

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'stark';

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
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

  /**
   * Test configuration links.
   */
  public function testConfigurationLinks() {
    // Check if Configure link is available on 'Extend' page.
    // Requires 'administer modules' permission.
    $this->drupalGet('admin/modules');
    $this->assertSession()->responseContains('admin/config/services/acquia-lift');

    // Check if Configure link is available on 'Status Reports' page.
    // NOTE: Link is only shown without a configured Acquia Lift credential.
    // Requires 'administer site configuration' permission.
    $this->drupalGet('admin/reports/status');
    $this->assertSession()->responseContains('admin/config/services/acquia-lift');
  }

  /**
   * Test admin settings form.
   */
  public function testAdminSettingsForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/services/acquia-lift');
    $this->assertSession()->responseContains(t('Acquia Lift Settings'));

    // Get all the valid settings, and massage them into form $edit array.
    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $field_mappings_settings = $this->getValidFieldMappingsSettings();
    $udf_person_settings = $this->getValidUdfPersonMappingsFormData();
    $udf_touch_settings = $this->getValidUdfTouchMappingsFormData();
    $udf_event_settings = $this->getValidUdfEventMappingsFormData();
    $visibility_settings = $this->getValidVisibilitySettings();
    $advanced_settings = $this->getValidAdvancedSettings();

    $edit = [];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');
    $edit += $this->convertToPostFormSettings($identity_settings, 'identity');
    $edit += $this->convertToPostFormSettings($field_mappings_settings, 'field_mappings');
    $edit += $this->convertToPostFormSettings($udf_person_settings, 'udf_person_mappings');
    $edit += $this->convertToPostFormSettings($udf_touch_settings, 'udf_touch_mappings');
    $edit += $this->convertToPostFormSettings($udf_event_settings, 'udf_event_mappings');
    $edit += $this->convertToPostFormSettings($visibility_settings, 'visibility');
    $edit += $this->convertToPostFormSettings($advanced_settings, 'advanced');
    $edit_settings_count = count($edit);
    $expect_settings_count = 21;
    $this->drupalGet('admin/config/services/acquia-lift');

    // Post the edits.
    $this->submitForm($edit, new TranslatableMarkup('Save configuration'));

    // Assert error messages are set for required fields and unreachable URLs.
    $this->assertSession()->pageTextContains(t('The Acquia Lift module requires a valid Account ID, Site ID, and Assets URL to complete activation.'));
    $this->assertSession()->pageTextContains(t('Acquia Lift module could not reach the specified Assets URL.'));
    $this->assertSession()->pageTextContains(t('Acquia Lift module could not reach the specified Decision API URL.'));

    // Assert all other fields.
    // Also count the asserted fields to make sure all are asserted.
    foreach ($edit as $name => $value) {
      $this->assertSession()->fieldValueEquals($name, $value);
    }
    $this->assertEquals($expect_settings_count, $edit_settings_count, 'The exact numbers of settings that were asserted should be ' . $expect_settings_count . '.');

    // Assert metatags are loaded in the header.
    $this->drupalGet('node/90210');
  }

  /**
   * Test Metatags and Script Tag.
   */
  public function testMetatagsAndScriptTag() {
    $this->setValidSettings();

    // Assert metatags are loaded in the header.
    $this->drupalGet('node/90210', ['query' => ['my_identity_parameter' => 'an_identity']]);
    $this->assertSession()->responseContains('an_identity');
    $this->assertSession()->responseContains('acquia_lift:page_type');
    $this->assertSession()->responseContains('node page');
    $this->assertSession()->responseContains('acquia_lift:account_id');
    $this->assertSession()->responseContains('AccountId1');
    $this->assertSession()->responseContains('acquia_lift:bootstrapMode');
    $this->assertSession()->responseContains('manual');
    $this->assertSession()->responseContains('acquia_lift:contentReplacementMode');
    $this->assertSession()->responseContains('customized');
    $this->assertSession()->responseNotContains('acquia_lift:content_origins');
    // Assert Lift JavaScript tag is async-loaded on the page.
    $this->assertSession()->responseContains('AssetsUrl1');
    $this->assertSession()->responseContains('async');

    // Update settings to include content_origins.
    $this->drupalGet('admin/config/services/acquia-lift');

    // Get all the valid settings, and massage them into form $edit array.
    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $field_mappings_settings = $this->getValidFieldMappingsSettings();
    $udf_person_settings = $this->getValidUdfPersonMappingsFormData();
    $udf_touch_settings = $this->getValidUdfTouchMappingsFormData();
    $udf_event_settings = $this->getValidUdfEventMappingsFormData();
    $visibility_settings = $this->getValidVisibilitySettings();
    $advanced_settings = $this->getValidAdvancedSettings("2a14f4d4-650e-47c2-a55f-25f29949b38e\r\n1b5bd833-b479-4d30-8ac2-331499acca9a\r\n81fbe311-c638-4ced-9db6-5a30889c925e\r\n5245d03d-32d5-4506-bc86-081022c7ae80\r\n");

    $edit = [];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');
    $edit += $this->convertToPostFormSettings($identity_settings, 'identity');
    $edit += $this->convertToPostFormSettings($field_mappings_settings, 'field_mappings');
    $edit += $this->convertToPostFormSettings($udf_person_settings, 'udf_person_mappings');
    $edit += $this->convertToPostFormSettings($udf_touch_settings, 'udf_touch_mappings');
    $edit += $this->convertToPostFormSettings($udf_event_settings, 'udf_event_mappings');
    $edit += $this->convertToPostFormSettings($visibility_settings, 'visibility');
    $edit += $this->convertToPostFormSettings($advanced_settings, 'advanced');
    $this->drupalGet('admin/config/services/acquia-lift');

    // Post the edits.
    $this->submitForm($edit, new TranslatableMarkup('Save configuration'));

    $this->drupalGet('node/90210', ['query' => ['my_identity_parameter' => 'an_identity']]);
    $this->assertSession()->responseContains('an_identity');
    $this->assertSession()->responseContains('acquia_lift:page_type');
    $this->assertSession()->responseContains('node page');
    $this->assertSession()->responseContains('acquia_lift:account_id');
    $this->assertSession()->responseContains('AccountId1');
    $this->assertSession()->responseContains('acquia_lift:bootstrapMode');
    $this->assertSession()->responseContains('manual');
    $this->assertSession()->responseContains('acquia_lift:contentReplacementMode');
    $this->assertSession()->responseContains('customized');
    $this->assertSession()->responseContains('<meta itemprop="acquia_lift:content_origins" content="2a14f4d4-650e-47c2-a55f-25f29949b38e,1b5bd833-b479-4d30-8ac2-331499acca9a,81fbe311-c638-4ced-9db6-5a30889c925e,5245d03d-32d5-4506-bc86-081022c7ae80"');
    // Assert Lift JavaScript tag is async-loaded on the page.
    $this->assertSession()->responseContains('AssetsUrl1');
    $this->assertSession()->responseContains('async');
  }

}
