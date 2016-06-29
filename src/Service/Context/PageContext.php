<?php

namespace Drupal\acquia_lift\Service\Context;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\image\Entity\ImageStyle;

class PageContext {
  /**
   * Engagement score's default value.
   */
  const ENGAGEMENT_SCORE_DEFAULT = 1;

  /**
   * Field mappings.
   *
   * @var array
   */
  private $fieldMappings;

  /**
   * Thumbnail config.
   *
   * @var array
   */
  private $thumbnailConfig;

  /**
   * Taxonomy term storage.
   *
   * @var \Drupal\taxonomy\TermStorageInterface
   */
  private $taxonomyTermStorage;

  /**
   * Page context, with default value.
   *
   * @var array
   */
  private $pageContext = [
    'content_title' => 'Untitled',
    'content_type' => 'page',
    'page_type' => 'content page',
    'content_section' => '',
    'content_keywords' => '',
    'post_id' => '',
    'published_date' => '',
    'thumbnail_url' => '',
    'persona' => '',
    'engagement_score' => SELF::ENGAGEMENT_SCORE_DEFAULT,
    'author' => '',
  ];

  /**
   * Credential mapping.
   *
   * @var array
   */
  private static $CREDENTIAL_MAPPING = [
    'account_name' => 'account_id',
    'customer_site' => 'site_id',
    'api_url' => 'liftDecisionAPIURL',
    'oauth_url' => 'authEndpoint',
  ];

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   Entity type manager.
   */
  public function __construct(ConfigFactoryInterface $config_factory, EntityTypeManagerInterface $entity_type_manager) {
    $settings = $config_factory->get('acquia_lift.settings');
    $this->setPageContextCredential($settings->get('credential'));
    $this->fieldMappings = $settings->get('field_mappings');
    $this->thumbnailConfig = $settings->get('thumbnail');
    $this->taxonomyTermStorage = $entity_type_manager->getStorage('taxonomy_term');
  }

  /**
   * Set page context - credential.
   *
   * @param array $credential_settings
   *   Credential settings array.
   */
  private function setPageContextCredential($credential_settings) {
    foreach (SELF::$CREDENTIAL_MAPPING as $credential_key => $tag_name) {
      if (isset($credential_settings[$credential_key])) {
        $this->pageContext[$tag_name] = $credential_settings[$credential_key];
      }
    };
  }

  /**
   * Set page context by node.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  public function setByNode(EntityInterface $node) {
    $this->setNodeData($node);
    $this->setThumbnailUrl($node);
    $this->setFields($node);
  }

  /**
   * Set Node data.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setNodeData(EntityInterface $node) {
    $this->pageContext['content_type'] = $node->getType();
    $this->pageContext['content_title'] = $node->getTitle();
    $this->pageContext['published_date'] = $node->getCreatedTime();
    $this->pageContext['post_id'] = $node->id();
    $this->pageContext['author'] = $node->getOwner()->getUsername();
    $this->pageContext['page_type'] = 'node page';
  }

  /**
   * Set thumbnail URL.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setThumbnailUrl(EntityInterface $node) {
    $node_type = $node->getType();

    // Don't set, if no thumbnail has been configured.
    if (!isset($this->thumbnailConfig[$node_type])) {
      return;
    }
    $node_type_thumbnail_config = $this->thumbnailConfig[$node_type];
    $thumbnail_config_array = explode('->', $node_type_thumbnail_config['field']);

    $entity = $node;
    foreach ($thumbnail_config_array as $field_key) {
      // Don't set, if node has no such field or field has no such entity.
      if (empty($entity->{$field_key}->entity) ||
        method_exists($entity->{$field_key}, 'isEmpty') && $entity->{$field_key}->isEmpty()
      ) {
        return;
      }
      $entity = $entity->{$field_key}->entity;
    }

    if ($entity->bundle() !== 'file') {
      return;
    }
    $fileUri = $entity->getFileUri();

    // Process Image style.
    $image_style = ImageStyle::load($node_type_thumbnail_config['style']);
    // Return empty if no such image style.
    if (empty($image_style)) {
      return;
    }

    // Generate image URL.
    $thumbnail_uri = $image_style->buildUrl($fileUri);
    $this->pageContext['thumbnail_url'] = file_create_url($thumbnail_uri);
  }

  /**
   * Set fields.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   */
  private function setFields(EntityInterface $node) {
    $available_field_vocabulary_names = $this->getAvailableFieldVocabularyNames($node);
    $vocabulary_term_names = $this->getVocabularyTermNames($node);

    // Find Field Term names.
    foreach ($available_field_vocabulary_names as $page_context_name => $vocabulary_names) {
      $field_term_names = $this->getFieldTermNames($vocabulary_names, $vocabulary_term_names);
      $this->pageContext[$page_context_name] = implode(',', $field_term_names);
    }
  }

  /**
   * Get available Fields and their vocabulary names within the node.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   * @return array
   *   Node's available Fields' Vocabularies names.
   */
  private function getAvailableFieldVocabularyNames(EntityInterface $node) {
    $available_field_vocabulary_names = [];
    foreach ($this->fieldMappings as $page_context_name => $field_name) {
      if(!isset($node->{$field_name})) {
        continue;
      }
      $vocabulary_names = $node->{$field_name}->getSetting('handler_settings')['target_bundles'];
      $available_field_vocabulary_names[$page_context_name] = $vocabulary_names;
    }
    return $available_field_vocabulary_names;
  }

  /**
   * Get Vocabularies' Term names.
   *
   * @param \Drupal\Core\Entity\EntityInterface $node
   *   Node.
   * @return array
   *   Vocabularies' Term names.
   */
  private function getVocabularyTermNames(EntityInterface $node) {
    // Find the node's terms.
    $terms = $this->taxonomyTermStorage->getNodeTerms([$node->id()]);
    $node_terms = isset($terms[$node->id()]) ? $terms[$node->id()] : [];

    // Find the term names.
    $vocabulary_term_names = [];
    foreach ($node_terms as $term) {
      $vocabulary_id = $term->getVocabularyId();
      $term_name = $term->getName();
      $vocabulary_term_names[$vocabulary_id][] = $term_name;
    }
    return $vocabulary_term_names;
  }

  /**
   * Get Field's Term names.
   *
   * @param array $vocabulary_names
   *   Vocabulary names.
   * @param array $vocabulary_term_names
   *   Vocabulary Term names.
   * @return array
   *   Field Term names.
   */
  private function getFieldTermNames($vocabulary_names, $vocabulary_term_names) {
    $field_term_names = [];
    foreach ($vocabulary_names as $vocabulary_name) {
      if (!isset($vocabulary_term_names[$vocabulary_name])) {
        continue;
      }
      $field_term_names = array_merge($field_term_names, $vocabulary_term_names[$vocabulary_name]);
    }
    return array_unique($field_term_names);
  }

  /**
   * Get meta tags.
   *
   * @return array
   *   Meta tags.
   */
  public function getMetatags() {
    $metatags = [];

    // Generate meta tag render arrays.
    foreach ($this->pageContext as $name => $content) {
      $renderArray = $this->getMetaTagRenderArray($name, $content);
      $metatags[] = [$renderArray, $name];
    }

    return $metatags;
  }

  /**
   * Get the render array for a single meta tag.
   *
   * @param string $name
   *   The name for the meta tag
   * @param string $content
   *   The content for the meta tag
   * @return array
   *   The render array
   */
  private function getMetaTagRenderArray($name, $content) {
    return [
      '#type' => 'html_tag',
      '#tag' => 'meta',
      '#attributes' => [
        'itemprop' => 'acquia_lift:' . $name,
        'content' => $content,
      ],
    ];
  }

  /**
   * Set page context title.
   *
   * @param array|string $title
   *   Set page context title.
   */
  public function setPageContextTitle($title) {
    // If markup, strip tags and convert to string.
    if (is_array($title) && isset($title['#markup']) && isset($title['#allowed_tags'])) {
      $allowed_tags = empty($title['#allowed_tags']) ? '' : '<' . implode('><', $title['#allowed_tags']) . '>';
      $title = strip_tags($title['#markup'], $allowed_tags);
    }
    // If still an array or empty, set title to empty.
    if (is_array($title) || empty($title)) {
      $this->pageContext['content_title'] = '';
      return;
    }
    // Otherwise set title.
    $this->pageContext['content_title'] = $title;
  }
}
