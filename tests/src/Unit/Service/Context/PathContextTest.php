<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Context;

use Drupal\acquia_lift\Service\Context\PathContext;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;
use Drupal\Tests\UnitTestCase;

/**
 * Tests the Path Context.
 *
 * @coversDefaultClass \Drupal\acquia_lift\Service\Context\PathContext
 * @group acquia_lift
 */
class PathContextTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * Drupal Config Factory.
   *
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit\Framework\MockObject\MockObject
   */
  private $configFactory;

  /**
   * Lift Config.
   *
   * @var \Drupal\Core\Config\ImmutableConfig|\PHPUnit\Framework\MockObject\MockObject
   */
  private $settings;

  /**
   * Current path stack.
   *
   * @var \Drupal\Core\Path\CurrentPathStack|\PHPUnit\Framework\MockObject\MockObject
   */
  private $currentPathStack;

  /**
   * Request stack.
   *
   * @var \Symfony\Component\HttpFoundation\RequestStack|\PHPUnit\Framework\MockObject\MockObject
   */
  private $requestStack;

  /**
   * Path matcher.
   *
   * @var \Drupal\acquia_lift\Service\Helper\PathMatcher|\PHPUnit\Framework\MockObject\MockObject
   */
  private $pathMatcher;

  /**
   * Request.
   *
   * @var \Symfony\Component\HttpFoundation\Request|\PHPUnit\Framework\MockObject\MockObject
   */
  private $request;

  /**
   * The visibility setting.
   *
   * @var array|string[]
   */
  protected $visibilitySettings;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->configFactory = $this->createMock('Drupal\Core\Config\ConfigFactoryInterface');
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();
    $this->currentPathStack = $this->getMockBuilder('Drupal\Core\Path\CurrentPathStack')
      ->disableOriginalConstructor()
      ->getMock();
    $this->requestStack = $this->createMock('Symfony\Component\HttpFoundation\RequestStack');
    $this->pathMatcher = $this->getMockBuilder('Drupal\acquia_lift\Service\Helper\PathMatcher')
      ->disableOriginalConstructor()
      ->getMock();
    $this->request = $this->createMock('Symfony\Component\HttpFoundation\Request');

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);

    $this->visibilitySettings = $this->getValidVisibilitySettings();
    $this->currentPathStack->expects($this->once())
      ->method('getPath')
      ->willReturn('my_current_path');
  }

  /**
   * Tests the shouldAttach() method.
   *
   * @param bool $set_invalid_credential
   *   Set Invalid Credentials.
   * @param bool $do_match_pattern
   *   Check if the pattern matches.
   * @param array $expect_should_attach
   *   Expected attachment.
   *
   * @covers ::shouldAttach
   *
   * @dataProvider providerTestShouldAttach
   */
  public function testShouldAttach($set_invalid_credential, $do_match_pattern, array $expect_should_attach) {
    $credential_settings = $this->getValidCredentialSettings();

    if ($set_invalid_credential) {
      $credential_settings['assets_url'] = '';
    }

    $this->requestStack->expects($this->once())
      ->method('getCurrentRequest')
      ->willReturn($this->request);

    $this->request->expects($this->once())
      ->method('getQueryString')
      ->willReturn('');

    $this->settings->expects($this->any())
      ->method('get')
      ->willReturnMap([
        ['visibility', $this->visibilitySettings],
        ['credential', $credential_settings],
        ['identity', $this->getValidIdentitySettings()],
      ]);

    $this->pathMatcher->expects($this->any())
      ->method('match')
      ->with('my_current_path', "/admin\n/admin/*\n/batch\n/node/add*\n/node/*/*\n/user/*\n/block/*")
      ->willReturn($do_match_pattern);

    $path_context = new PathContext($this->configFactory, $this->currentPathStack, $this->requestStack, $this->pathMatcher);
    $should_attach = $path_context->shouldAttach();

    $this->assertEquals($expect_should_attach, $should_attach);
  }

  /**
   * Data provider for testShouldAttach().
   */
  public function providerTestShouldAttach() {
    $set_invalid_credential = TRUE;
    $set_valid_credential = FALSE;
    $do_match_pattern = TRUE;
    $no_match_pattern = FALSE;
    $expect_should_attach = TRUE;
    $expect_should_not_attach = FALSE;

    $data['invalid credential'] = [
      $set_invalid_credential,
      $no_match_pattern,
      $expect_should_not_attach,
    ];
    $data['valid credential, matched pattern'] = [
      $set_valid_credential,
      $do_match_pattern,
      $expect_should_not_attach,
    ];
    $data['valid credential, no matched pattern'] = [
      $set_valid_credential,
      $no_match_pattern,
      $expect_should_attach,
    ];

    return $data;
  }

  /**
   * Tests the populate() method, populateHtmlHead() sub method.
   *
   * @param string $query_parameter_string
   *   Query Parameters.
   * @param bool $capture_identity
   *   Identity.
   * @param bool $do_set_user
   *   Check to see if user is set.
   * @param array $expect_cache
   *   Expected Cache values.
   * @param array $expect_html_head
   *   Expected HTML headers.
   *
   * @covers ::setContextIdentityByUser
   * @covers ::populate
   *
   * @dataProvider providerTestPopulateHtmlHeadIdentities
   */
  public function testPopulateHtmlHeadIdentities($query_parameter_string, $capture_identity, $do_set_user, array $expect_cache, array $expect_html_head) {
    $this->requestStack->expects($this->once())
      ->method('getCurrentRequest')
      ->willReturn($this->request);
    $this->request->expects($this->once())
      ->method('getQueryString')
      ->willReturn($query_parameter_string);

    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $identity_settings['capture_identity'] = $capture_identity;

    $this->settings->expects($this->any())
      ->method('get')
      ->willReturnMap([
        ['visibility', $this->visibilitySettings],
        ['credential', $credential_settings],
        ['identity', $identity_settings],
      ]);

    $path_context = new PathContext($this->configFactory, $this->currentPathStack, $this->requestStack, $this->pathMatcher);

    if ($do_set_user) {
      $user = $this->createMock('Drupal\user\UserInterface');
      $user->expects($this->exactly((int) $capture_identity))
        ->method('getEmail')
        ->willReturn('a_user_email');
      $path_context->setContextIdentityByUser($user);
    }

    $page = [];
    $path_context->populate($page);

    $this->assertEquals($expect_cache, $page['#cache']['contexts']);
    $this->assertEquals($expect_html_head, $page['#attached']['html_head']);
  }

  /**
   * Data provider for testPopulateHtmlHeadIdentities().
   */
  public function providerTestPopulateHtmlHeadIdentities() {
    $no_query_parameter_string = '';
    $full_query_parameter_string = 'my_identity_parameter=query_identity&my_identity_type_parameter=query_identity_type&other=other';
    $partial_query_parameter_string = 'my_identity_parameter=query_identity&other=other';
    $no_capture_identity = FALSE;
    $do_capture_identity = TRUE;
    $no_set_user = FALSE;
    $do_set_user = TRUE;
    $expect_cache_identity_and_identity_type = [
      'url.query_args:my_identity_parameter',
      'url.query_args:my_identity_type_parameter',
    ];
    $expect_html_head_empty = NULL;
    $expect_identity_of_full_query_string = [[
      [
        '#type' => 'html_tag',
        '#tag' => 'meta',
        '#attributes' => [
          'itemprop' => 'acquia_lift:identity:query_identity_type',
          'content' => 'query_identity',
        ],
      ],
      'identity:query_identity_type',
    ]];
    $expect_identity_of_partial_query_string = [[
      [
        '#type' => 'html_tag',
        '#tag' => 'meta',
        '#attributes' => [
          'itemprop' => 'acquia_lift:identity:my_default_identity_type',
          'content' => 'query_identity',
        ],
      ],
      'identity:my_default_identity_type',
    ]];
    $expect_identity_of_user = [[
      [
        '#type' => 'html_tag',
        '#tag' => 'meta',
        '#attributes' => [
          'itemprop' => 'acquia_lift:identity:email',
          'content' => 'a_user_email',
        ],
      ],
      'identity:email',
    ]];

    $expect_identity_of_full_query_string_and_user = array_merge($expect_identity_of_full_query_string, $expect_identity_of_user);

    $data['no query, no capture, no user'] = [
      $no_query_parameter_string,
      $no_capture_identity,
      $no_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_html_head_empty,
    ];
    $data['no query, no capture, yes user'] = [
      $no_query_parameter_string,
      $no_capture_identity,
      $do_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_html_head_empty,
    ];
    $data['no query, do capture, yes user'] = [
      $no_query_parameter_string,
      $do_capture_identity,
      $do_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_identity_of_user,
    ];
    $data['yes query, no capture, no user'] = [
      $full_query_parameter_string,
      $no_capture_identity,
      $no_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_identity_of_full_query_string,
    ];
    $data['yes query (but partial), no capture, no user'] = [
      $partial_query_parameter_string,
      $no_capture_identity,
      $no_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_identity_of_partial_query_string,
    ];
    $data['yes query, no capture, yes user'] = [
      $full_query_parameter_string,
      $no_capture_identity,
      $do_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_identity_of_full_query_string,
    ];
    $data['yes query, do capture, yes user'] = [
      $full_query_parameter_string,
      $do_capture_identity,
      $do_set_user,
      $expect_cache_identity_and_identity_type,
      $expect_identity_of_full_query_string_and_user,
    ];

    return $data;
  }

  /**
   * Tests the populate() method, "set identity and identity type" sub routine.
   *
   * @param array $identity_settings
   *   The identity settings.
   * @param int $expect_set_cache
   *   Expected Cache value.
   * @param array $expect_cache_context
   *   Expected Cache context.
   *
   * @covers ::populate
   *
   * @dataProvider providerTestPopulateCache
   */
  public function testPopulateCache($identity_settings, $expect_set_cache, $expect_cache_context) {
    $this->requestStack->expects($this->exactly($expect_set_cache))
      ->method('getCurrentRequest')
      ->willReturn($this->request);
    $this->request->expects($this->exactly($expect_set_cache))
      ->method('getQueryString')
      ->willReturn('querystring');
    $this->settings->expects($this->any())
      ->method('get')
      ->willReturnMap([
        ['visibility', $this->visibilitySettings],
        ['credential', []],
        ['identity', $identity_settings],
      ]);

    $path_context = new PathContext($this->configFactory, $this->currentPathStack, $this->requestStack, $this->pathMatcher);

    $page = [];
    $path_context->populate($page);

    $cache_context = [];
    if (isset($page['#cache']['contexts'])) {
      $cache_context = $page['#cache']['contexts'];
    }

    $this->assertEquals($expect_cache_context, $cache_context);
  }

  /**
   * Data provider for testPopulateCache().
   */
  public function providerTestPopulateCache() {
    $identity_setting_empty = [
      'identity_parameter' => '',
      'identity_type_parameter' => '',
      'default_identity_type' => '',
    ];
    $identity_setting_identity = [
      'identity_parameter' => 'my_identity_parameter',
      'identity_type_parameter' => '',
      'default_identity_type' => '',
    ];
    $identity_setting_identity_type = [
      'identity_parameter' => '',
      'identity_type_parameter' => 'my_identity_type_parameter',
      'default_identity_type' => '',
    ];
    $identity_setting_default_identity = [
      'identity_parameter' => '',
      'identity_type_parameter' => '',
      'default_identity_type' => 'my_default_identity_type',
    ];
    $identity_setting_identity_and_identity_type = [
      'identity_parameter' => 'my_identity_parameter',
      'identity_type_parameter' => 'my_identity_type_parameter',
      'default_identity_type' => '',
    ];
    $identity_setting_identity_and_default_identity = [
      'identity_parameter' => 'my_identity_parameter',
      'identity_type_parameter' => '',
      'default_identity_type' => 'my_default_identity_type',
    ];
    $identity_setting_identity_type_and_default_identity = [
      'identity_parameter' => '',
      'identity_type_parameter' => 'my_identity_type_parameter',
      'default_identity_type' => 'my_default_identity_type',
    ];
    $identity_setting_full = [
      'identity_parameter' => 'my_identity_parameter',
      'identity_type_parameter' => 'my_identity_type_parameter',
      'default_identity_type' => 'my_default_identity_type',
    ];
    $expect_set_cache_no = 0;
    $expect_set_cache_yes = 1;
    $expect_cache_context_empty = [];
    $expect_cache_context_identity = [
      'url.query_args:my_identity_parameter',
    ];
    $expect_cache_context_identity_and_identity_type = [
      'url.query_args:my_identity_parameter',
      'url.query_args:my_identity_type_parameter',
    ];

    $data['no identity, no identity type, no default identity'] = [
      $identity_setting_empty,
      $expect_set_cache_no,
      $expect_cache_context_empty,
    ];
    $data['yes identity, no identity type, no default identity'] = [
      $identity_setting_identity,
      $expect_set_cache_yes,
      $expect_cache_context_identity,
    ];
    $data['no identity, yes identity type, no default identity'] = [
      $identity_setting_identity_type,
      $expect_set_cache_no,
      $expect_cache_context_empty,
    ];
    $data['no identity, no identity type, yes default identity'] = [
      $identity_setting_default_identity,
      $expect_set_cache_no,
      $expect_cache_context_empty,
    ];
    $data['yes identity, yes identity type, no default identity'] = [
      $identity_setting_identity_and_identity_type,
      $expect_set_cache_yes,
      $expect_cache_context_identity_and_identity_type,
    ];
    $data['yes identity, no identity type, yes default identity'] = [
      $identity_setting_identity_and_default_identity,
      $expect_set_cache_yes,
      $expect_cache_context_identity,
    ];
    $data['no identity, yes identity type, yes default identity'] = [
      $identity_setting_identity_type_and_default_identity,
      $expect_set_cache_no,
      $expect_cache_context_empty,
    ];
    $data['yes identity, yes identity type, yes default identity'] = [
      $identity_setting_full,
      $expect_set_cache_yes,
      $expect_cache_context_identity_and_identity_type,
    ];

    return $data;
  }

}
