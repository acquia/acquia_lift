<?php

namespace Drupal\Tests\acquia_lift_publisher\Functional\Form;

use Drupal\Core\Url;
use Drupal\Tests\BrowserTestBase;

/**
 * Test for the EntityViewModeConfigurationForm.
 *
 * @covers \Drupal\acquia_lift_publisher\Form\EntityViewModeConfigurationForm
 *
 * @group acquia_lift
 *
 * @package Drupal\Tests\acquia_lift_publisher\Functional
 */
class EntityViewModeConfigurationFormTest extends BrowserTestBase {

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'stark';

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'system',
    'config',
    'user',
    'block',
    'node',
    'acquia_lift',
    'acquia_lift_publisher',
  ];

  /**
   * The Admin User.
   *
   * @var \Drupal\user\Entity\User
   */
  private $adminUser;

  /**
   * The Non-Admin user.
   *
   * @var \Drupal\user\Entity\User
   */
  private $notAdminUser;

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();
    $this->adminUser = $this->drupalCreateUser([
      'administer acquia lift',
    ]);
    $this->notAdminUser = $this->drupalCreateUser([]);

    $this->drupalCreateContentType(['type' => 'page', 'name' => t('Basic page')]);
    $this->drupalCreateContentType(['type' => 'article', 'name' => t('Article')]);
  }

  /**
   * Tests EntityViewModeConfigurationForm.
   *
   * @throws \Behat\Mink\Exception\ElementNotFoundException
   * @throws \Behat\Mink\Exception\ExpectationException
   */
  public function testEntityViewModeConfigurationForm() {
    $formUrl = Url::fromRoute('acquia_lift_publisher.entity_config_form');

    $this->drupalLogin($this->notAdminUser);
    $this->drupalGet($formUrl);
    $this->assertSession()->statusCodeEquals(403);
    $this->drupalLogout();

    $this->drupalLogin($this->adminUser);
    $this->drupalGet($formUrl);
    $this->assertSession()->statusCodeEquals(200);

    $page = $this->getSession()
      ->getPage();

    $this->assertTrue($page->hasUncheckedField('options[user][user][default]'));

    $bundles = ['page', 'article'];
    $view_modes = ['default', 'full', 'teaser'];
    foreach ($bundles as $bundle) {
      foreach ($view_modes as $view_mode) {
        $locator = sprintf("options[node][%s][%s]", $bundle, $view_mode);
        $this->assertTrue($page->hasUncheckedField($locator));
        $page->checkField($locator);
      }
    }

    $page->pressButton('edit-submit');

    foreach ($bundles as $bundle) {
      foreach ($view_modes as $view_mode) {
        $locator = sprintf("options[node][%s][%s]", $bundle, $view_mode);
        $this->assertTrue($page->hasCheckedField($locator));
      }
    }
  }

}
