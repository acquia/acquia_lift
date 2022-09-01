<?php

namespace Drupal\Tests\acquia_lift_publisher\Functional;

use Drupal\Core\Url;
use Drupal\Tests\BrowserTestBase;
use Drupal\Tests\image\Kernel\ImageFieldCreationTrait;
use Drupal\user\UserInterface;

/**
 * Tests basic form elements of ContentPublishingForm.
 *
 * @package Drupal\Tests\acquia_lift_publisher\Functional
 *
 * @coversDefaultClass \Drupal\acquia_lift_publisher\Form\ContentPublishingForm
 * @group acquia_lift_publisher
 */
class ContentPublishingFormTest extends BrowserTestBase {

  use ImageFieldCreationTrait;

  /**
   * Authenticated user.
   *
   * @var \Drupal\user\UserInterface
   */
  private $user;

  /**
   * Authenticated user with permission.
   *
   * @var \Drupal\user\UserInterface
   */
  private $userWithPermission;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'acquia_contenthub',
    'acquia_contenthub_publisher',
    'acquia_lift',
    'acquia_lift_publisher',
    'filter',
    'text',
    'node',
  ];

  /**
   * {@inheritdoc}
   */
  protected $defaultTheme = 'stark';

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();

    $this->user = $this->createUser([], 'user1');
    $this->userWithPermission = $this->createUser(['administer acquia lift'], 'user2');
  }

  /**
   * Tests form route and the rendered fields.
   */
  public function testFormAccessibility(): void {
    $this->assertFormAccessibitlity($this->user, 403, 'User does not have the permission to view the form');
    $this->assertFormAccessibitlity($this->userWithPermission, 200, 'User has the permission to view the form');
  }

  /**
   * Asserts if form is reachable for the given user.
   *
   * @param \Drupal\user\UserInterface $user
   *   The current user.
   * @param int $expected
   *   The expected status code.
   * @param string $message
   *   The message to display in case of the failing test.
   */
  private function assertFormAccessibitlity(UserInterface $user, int $expected, string $message): void {
    $this->drupalLogin($user);
    $session = $this->getSession();
    $this->drupalGet(Url::fromRoute('acquia_lift_publisher.entity_config_form'));
    $this->assertEquals($session->getStatusCode(), $expected, $message);
  }

  /**
   * Tests the available form fields.
   *
   * @throws \Behat\Mink\Exception\ExpectationException
   */
  public function testFormFields() {
    $this->drupalCreateContentType([
      'type' => 'article',
    ]);

    $this->drupalLogin($this->userWithPermission);
    $session = $this->assertSession();

    $url = Url::fromRoute('acquia_lift_publisher.entity_config_form')->toString();
    $this->drupalGet($url);
    $view_mode_field = 'options[node][article][full]';
    $preview_image = 'options[node][article][acquia_lift_preview_image]';
    $render_role = 'render_role';
    $synchronization = 'personalized_content_only';

    $session->pageTextContains('Acquia Lift Publisher Settings');

    $session->fieldExists($view_mode_field);
    // No image field has been added, therefore this field should not be seen.
    $session->fieldNotExists($preview_image);
    $session->fieldExists($render_role);
    $session->fieldExists($synchronization);

    $session->elementTextContains('css', '.form-item-render-role', 'Render role');
    $session->elementTextContains('css', '.form-item-render-role',
      'The role to use when rendering entities for personalization.'
    );

    $session->elementTextContains('css', '#edit-sync-settings', 'Push personalization content only');
    $session->elementTextContains('css', '#edit-sync-settings',
      'Check this option if this site is used for pushing content to Acquia Lift. Disable this option if this site is used for pushing content to both Acquia Lift and Content Hub. (Default is enabled)'
    );

    $session->elementTextContains('css', '#edit-sync-settings', 'Export content immediately');
    $session->elementTextContains('css', '#edit-sync-settings',
      'Check this option if the export queue should be run immediately after saving content. Disable this option if the export queue is being run separately. (Default is enabled)'
    );

    // Initial configuration values.
    $session->checkboxNotChecked($view_mode_field);
    $session->fieldValueEquals($render_role, 'anonymous');
    $session->checkboxChecked($synchronization);

    // Add image field, now the form element should be visible.
    $this->createImageField('image', 'article');
    $this->getSession()->reload();
    $session->fieldExists($preview_image);
    $this->drupalGet($url);

    $this->submitForm([
      $view_mode_field => TRUE,
      $preview_image => 'image',
      $render_role => 'authenticated',
      $synchronization => FALSE,
    ], 'Save configuration');

    $session->checkboxChecked($view_mode_field);
    $session->fieldValueEquals($preview_image, 'image');
    $session->fieldValueEquals($render_role, 'authenticated');
    $session->checkboxNotChecked($synchronization);
  }

}
