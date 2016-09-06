<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\PathMatcher;

/**
 * PathMatcher Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\PathMatcher
 * @group acquia_lift
 */
class PathMatcherTest extends UnitTestCase {
  /**
   * Alias manager.
   *
   * @var \Drupal\Core\Path\AliasManagerInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $aliasManager;

  /**
   * Path matcher.
   *
   * @var \Drupal\Core\Path\PathMatcherInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $basePathMatcher;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->aliasManager = $this->getMock('Drupal\Core\Path\AliasManagerInterface');
    $this->basePathMatcher = $this->getMock('Drupal\Core\Path\PathMatcherInterface');
  }

  /**
   * Tests the match() method - no match.
   *
   * @covers ::match
   */
  public function testMatchNoMatch() {
    $this->basePathMatcher->expects($this->exactly(2))
      ->method('matchPath')
      ->willReturn(FALSE);

    $pathMatcher = new PathMatcher($this->aliasManager, $this->basePathMatcher);
    $is_matched = $pathMatcher->match('A_PATH', 'A_PATTERN');
    $this->assertFalse($is_matched);
  }

  /**
   * Tests the match() method - path is matched.
   *
   * @covers ::match
   */
  public function testMatchPathMatched() {
    $this->basePathMatcher->expects($this->once())
      ->method('matchPath')
      ->with('a_path', 'a_pattern')
      ->willReturn(TRUE);
    $this->aliasManager->expects($this->never())
      ->method('getAliasByPath');

    $pathMatcher = new PathMatcher($this->aliasManager, $this->basePathMatcher);
    $is_matched = $pathMatcher->match('A_PATH', 'A_PATTERN');
    $this->assertTrue($is_matched);
  }

  /**
   * Tests the match() method - path's alias is matched.
   *
   * @covers ::match
   */
  public function testMatchAliasMatched() {
    $this->basePathMatcher->expects($this->at(0))
      ->method('matchPath')
      ->with('a_path', 'a_pattern')
      ->willReturn(FALSE);
    $this->aliasManager->expects($this->once())
      ->method('getAliasByPath')
      ->with('a_path')
      ->willReturn('AN_ALIAS');
    $this->basePathMatcher->expects($this->at(1))
      ->method('matchPath')
      ->with('an_alias', 'a_pattern')
      ->willReturn(TRUE);

    $pathMatcher = new PathMatcher($this->aliasManager, $this->basePathMatcher);
    $is_matched = $pathMatcher->match('A_PATH', 'A_PATTERN');
    $this->assertTrue($is_matched);
  }
}
