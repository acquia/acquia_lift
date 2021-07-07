<?php

namespace Drupal\Tests\acquia_perz\Traits;

use Drupal\block_content\Entity\BlockContent;
use Drupal\block_content\Entity\BlockContentType;

/**
 * Provides common helper methods for Custom block's related tests.
 */
trait CustomBlockTestTrait {

  /**
   * Returns a new block type with random properties.
   *
   * @return \Drupal\block_content\BlockContentTypeInterface
   *   A block type used for testing.
   */
  public function createCustomBlockType() {
    $block_type = BlockContentType::create([
      'id' => $this->randomMachineName(32),
      'label' => $this->randomMachineName(64),
      'description' => $this->randomMachineName(64),
    ]);
    $block_type->save();
    return $block_type;
  }

  /**
   * Returns a new custom block with random properties.
   *
   * @return \Drupal\block_content\BlockContentInterface
   *   A custom block used for testing.
   */
  public function createCustomBlock($block_type) {
    $custom_block = BlockContent::create([
      'type' => $block_type->id(),
      'info' => $this->randomMachineName(32),
      'body' => [
        'value' => $this->randomMachineName(64),
        'format' => 'full_html',
      ],
    ]);
    $custom_block->save();
    return $custom_block;
  }

}
