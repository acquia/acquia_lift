<?php

namespace Drupal\Tests\acquia_lift_inspector\FunctionalJavascript;

use Drupal\acquia_lift\Tests\SettingsDataTrait;
use Drupal\FunctionalJavascriptTests\JavascriptTestBase;
use Drupal\simpletest\ContentTypeCreationTrait;
use Drupal\simpletest\NodeCreationTrait;

/**
 * Tests for the JS that transforms widgets into form elements.
 *
 * @group facets
 */
class InspectorJsTest extends JavascriptTestBase {

  use SettingsDataTrait;
  use ContentTypeCreationTrait;
  use NodeCreationTrait;

  /**
   * {@inheritdoc}
   */
  public static $modules = [
    'acquia_lift',
    'acquia_lift_inspector'
  ];

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    // Ensure an `article` node type exists.
    $this->createContentType(['type' => 'article']);

  }

  /**
   * Get a valid credential settings array.
   *
   * @return array
   *   A valid credential settings array.
   */
  private function getActualJSCredentialSettings() {
    return [
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      // Only the JS needs to be valid. Otherwise the JS test won't work.
      'assets_url' => 'https://lift3assets.dev.lift.acquia.com/LEX-1647-dev',
      'decision_api_url' => 'decision_api_url_1',
      'oauth_url' => 'oauth_url_1//////authorize',
    ];
  }

  /**
   * Tests JS interactions in the admin UI.
   */
  public function testShowDebug() {
    // Create the users used for the tests.
    $admin_user = $this->drupalCreateUser([
      'administer acquia lift',
    ]);
    $this->drupalLogin($admin_user);

    // Set valid settings
    $this->setValidSettings();
    // Set an actual JS lift.js file in the settings for the inspector to work.
    $settings = $this->config('acquia_lift.settings');
    $settings->set('credential', $this->getActualJSCredentialSettings());
    $settings->save();

    // Check if Acquia Lift Link is available on the node that we created
    $node = $this->createNode([
      'type' => 'article'
    ]);

    // Assert that the Acquia Lift link ID is present in the HTML.
    // This also verifies if the specific render cache is cleared.
    $this->drupalGet($node->toUrl()->toString());

    // Confirm that AcquiaLift loaded.
    $javascript = <<<JS
    (function(){
      return Object.keys(window.AcquiaLift).length > 0;
    }());
JS;

    $this->assertJsCondition($javascript);

    // Should not show the inspector
    $page = $this->getSession()->getPage();
    $subNav = $page->findById('lift-inspector');
    var_dump($subNav);

    $this->getSession()->wait(6000, "Drupal.acquiaLiftInspector.showModal();");
    $page = $this->getSession()->getPage();

    // Should show the inspector
    $subNav = $page->findById('lift-inspector');
    $this->assertNotEmpty($subNav);
    var_dump($subNav->getHtml());

  }
}
