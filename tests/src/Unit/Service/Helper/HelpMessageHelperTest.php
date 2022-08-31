<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Helper;

use Drupal\acquia_lift\Service\Helper\HelpMessageHelper;
use Drupal\Tests\UnitTestCase;

/**
 * HelpMessageHelper Test.
 *
 * @coversDefaultClass \Drupal\acquia_lift\Service\Helper\HelpMessageHelper
 * @group acquia_lift
 */
class HelpMessageHelperTest extends UnitTestCase {

  /**
   * Tests the getMessage() method - AdminSettingsForm.
   *
   * @param string $route_name
   *   The Route Name.
   * @param string $has_message
   *   Checks for a message.
   *
   * @covers ::getMessage
   *
   * @dataProvider providerRouteNames
   */
  public function testGetMessageAdminSettingsFormNoApiUrl($route_name, $has_message) {
    $help_message_helper = new HelpMessageHelper();
    $help_message_helper->setStringTranslation($this->getStringTranslationStub());
    $message = $help_message_helper->getMessage($route_name);
    if ($has_message) {
      $this->assertEquals('You can find more info in <a href="https://docs.acquia.com/lift" target="_blank">Documentation</a>.', $message->render());
    }
    else {
      $this->assertNull($message);
    }
  }

  /**
   * Data provider to produce route names.
   */
  public function providerRouteNames() {
    $data = [];

    $data['help page, has message'] = ['help.page.acquia_lift', TRUE];
    $data['admin settings form, has message'] = ['acquia_lift.admin_settings_form', TRUE];
    $data['admin settings form, has no message'] = ['acquia_contenthub.admin_settings_form', FALSE];

    return $data;
  }

}
