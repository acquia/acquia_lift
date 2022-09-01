<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Helper;

use Drupal\acquia_lift\Service\Helper\PathMatcher;
use Drupal\Tests\UnitTestCase;

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
   * @var \Drupal\path_alias\AliasManagerInterface|\PHPUnit\Framework\MockObject\MockObject
   */
  private $aliasManager;

  /**
   * Path matcher.
   *
   * @var \Drupal\Core\Path\PathMatcherInterface|\PHPUnit\Framework\MockObject\MockObject
   */
  private $basePathMatcher;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->aliasManager = $this->createMock('Drupal\path_alias\AliasManagerInterface');
    $this->basePathMatcher = $this->createMock('Drupal\Core\Path\PathMatcherInterface');
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
    $this->basePathMatcher->expects($this->exactly(2))
      ->method('matchPath')
      ->withConsecutive(['a_path', 'a_pattern'], ['an_alias', 'a_pattern'])
      ->willReturnOnConsecutiveCalls(FALSE, TRUE);
    $this->aliasManager->expects($this->once())
      ->method('getAliasByPath')
      ->with('a_path')
      ->willReturn('AN_ALIAS');

    $pathMatcher = new PathMatcher($this->aliasManager, $this->basePathMatcher);
    $is_matched = $pathMatcher->match('A_PATH', 'A_PATTERN');
    $this->assertTrue($is_matched);
  }

}
