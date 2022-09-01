<?php

namespace Drupal\Tests\acquia_lift_inspector\FunctionalJavascript;

use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;
use Drupal\FunctionalJavascriptTests\WebDriverTestBase;
use Drupal\Tests\node\Traits\ContentTypeCreationTrait;
use Drupal\Tests\node\Traits\NodeCreationTrait;

/**
 * Tests for the JS that transforms widgets into form elements.
 *
 * @group acquia_lift_inspector
 */
class InspectorJsTest extends WebDriverTestBase {

  use SettingsDataTrait;
  use ContentTypeCreationTrait;
  use NodeCreationTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'acquia_lift',
    'acquia_lift_inspector',
  ];

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'stark';

  /**
   * {@inheritdoc}
   */
  public function setUp(): void {
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
      'assets_url' => 'https://lift3assets.dev.lift.acquia.com/latest',
      'decision_api_url' => 'decision_api_url_1',
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

    // Set valid settings.
    $this->setValidSettings();
    // Set an actual JS lift.js file in the settings for the inspector to work.
    $settings = $this->config('acquia_lift.settings');
    $settings->set('credential', $this->getActualJSCredentialSettings());
    $settings->save();

    // Check if Acquia Lift Link is available on the node that we created.
    $node = $this->createNode([
      'type' => 'article',
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

    // Should not show the inspector.
    $page = $this->getSession()->getPage();
    $inspector = $page->findById('lift-inspector');
    $this->assertEmpty($inspector);

    // Open the inspector.
    $this->getSession()->wait(6000, "Drupal.acquiaLiftInspector.showModal();");
    $page = $this->getSession()->getPage();

    // Should show the inspector.
    $inspector = $page->findById('lift-inspector');
    $this->assertNotEmpty($inspector);

    $accountId = $inspector->find('css', '#account-id a')->getText();
    $this->assertEquals($accountId, 'AccountId1');

    $accountLink = $inspector->find('css', '#account-id a')->getAttribute('href');
    $this->assertEquals($accountLink, 'https://us-east-1.lift.acquia.com#person:accountId=AccountId1');

    $siteId = $inspector->find('css', '#site-id')->getText();
    $this->assertEquals($siteId, 'SiteId1');

    $identity = $inspector->find('css', '#identity')->getText();
    $this->assertEquals($identity, 'No tracking id available.');

    $identity = $inspector->find('css', '#user-segments p')->getText();
    $this->assertEquals($identity, 'No segment(s) available.');

    $identity = $inspector->find('css', '#decisions p')->getText();
    $this->assertEquals($identity, 'No decision(s) made.');

    $identity = $inspector->find('css', '#captures p')->getText();
    $this->assertEquals($identity, 'No recent captures.');

  }

}
