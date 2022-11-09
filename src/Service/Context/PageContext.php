<?php

namespace Drupal\acquia_lift\Service\Context;

use Drupal\acquia_lift\Service\Helper\SettingsHelper;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Controller\TitleResolverInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Language\LanguageManagerInterface;
use Drupal\Core\Routing\RouteMatchInterface;
use Drupal\node\NodeInterface;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\RequestStack;
use Symfony\Component\Routing\Route;

/**
 * Page Context extending the Lift Context class.
 */
class PageContext extends BaseContext {

  /**
   * Engagement score's default value.
   */
  const ENGAGEMENT_SCORE_DEFAULT = 1;

  /**
   * Lift JavaScript file name.
   */
  const LIFT_JS_FILENAME = 'lift.js';

  /**
   * Field mappings.
   *
   * @var array
   */
  private $fieldMappings;

  /**
   * Udf Person mappings.
   *
   * @var array
   */
  private $udfPersonMappings;

  /**
   * Udf Event mappings.
   *
   * @var array
   */
  private $udfEventMappings;

  /**
   * Udf Touch mappings.
   *
   * @var array
   */
  private $udfTouchMappings;

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
  protected $htmlHeadContexts = [
    'content_title' => 'Untitled',
    'content_type' => 'page',
    'page_type' => 'content page',
    'context_language' => '',
    'content_section' => '',
    'content_keywords' => '',
    'post_id' => '',
    'content_uuid' => '',
    'published_date' => '',
    'persona' => '',
    'engagement_score' => self::ENGAGEMENT_SCORE_DEFAULT,
  ];

  /**
   * Assets URL.
   *
   * @var array
   */
  private $assetsUrl;

  /**
   * Credential mapping.
   *
   * @var array
   */
  private static $credentialMapping = [
    'account_id' => 'account_id',
    'site_id' => 'site_id',
    'assets_url' => 'liftAssetsURL',
    'decision_api_url' => 'liftDecisionAPIURL',
  ];

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   Entity type manager.
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *   The request stack.
   * @param \Drupal\Core\Routing\RouteMatchInterface $route_match
   *   The current route match.
   * @param \Drupal\Core\Controller\TitleResolverInterface $title_resolver
   *   The title resolver.
   * @param Drupal\Core\Language\LanguageManagerInterface $language_manager
   *   The language manager.
   */
  public function __construct(ConfigFactoryInterface $config_factory, EntityTypeManagerInterface $entity_type_manager, RequestStack $request_stack, RouteMatchInterface $route_match, TitleResolverInterface $title_resolver, LanguageManagerInterface $language_manager) {
    // Get all our settings.
    $settings = $config_factory->get('acquia_lift.settings');

    // Set Credential information.
    $credential_settings = $settings->get('credential');
    $this->assetsUrl = $credential_settings['assets_url'];
    $this->setContextCredential($credential_settings);

    // Set mapping information.
    $this->fieldMappings = $settings->get('field_mappings');
    $this->udfPersonMappings = $settings->get('udf_person_mappings') ?: [];
    $this->udfEventMappings = $settings->get('udf_touch_mappings') ?: [];
    $this->udfTouchMappings = $settings->get('udf_event_mappings') ?: [];

    // Set advanced configuration.
    $this->setContextAdvanced($settings->get('advanced'));

    // Set taxonomyTermStorage.
    $this->taxonomyTermStorage = $entity_type_manager->getStorage('taxonomy_term');

    // Set page context.
    $request = $request_stack->getCurrentRequest();
    $route = $route_match->getRouteObject();

    // Set node context.
    $this->setContextByNode($request);

    // Set base data (title + language)
    $this->setBaseData($request, $route, $title_resolver, $language_manager);
  }

  /**
   * Set page context by Node.
   *
   * @param \Symfony\Component\HttpFoundation\Request $request
   *   The request object.
   */
  private function setContextByNode(Request $request) {
    // If not a request to node, do nothing.
    if (!$request->attributes->has('node')) {
      return;
    }

    $node = $request->attributes->get('node');
    if (empty($node) || !$node instanceof NodeInterface) {
      return;
    }

    // Otherwise, set page context by node.
    $this->setNodeData($node);
    $this->setFields($node);
  }

  /**
   * Set page context - title.
   *
   * @param \Symfony\Component\HttpFoundation\Request $request
   *   The request object.
   * @param \Symfony\Component\Routing\Route $route
   *   The route object.
   * @param \Drupal\Core\Controller\TitleResolverInterface $titleResolver
   *   The title resolver.
   * @param Drupal\Core\Language\LanguageManagerInterface $languageManager
   *   The language manager.
   */
  private function setBaseData(Request $request, Route $route, TitleResolverInterface $titleResolver, LanguageManagerInterface $languageManager) {
    // Set language code
    // After investigation, there is no use case where the methods
    // 'getCurrentLanguage' and 'getId' would not exist within LanguageManager.
    // Drupal 8 will ALWAYS set a language code and would not be null,
    // therefore no checks are required.
    $this->htmlHeadContexts['context_language'] = $languageManager->getCurrentLanguage()->getId();

    // Get title.
    $title = $titleResolver->getTitle($request, $route);

    // Set title.
    // If markup, strip tags and convert to string.
    if (is_array($title) && isset($title['#markup']) && isset($title['#allowed_tags'])) {
      $allowed_tags = empty($title['#allowed_tags']) ? '' : '<' . implode('><', $title['#allowed_tags']) . '>';
      $title = strip_tags($title['#markup'], $allowed_tags);
    }
    // If still an array or empty, leave title at "Untitled".
    if (is_array($title) || empty($title)) {
      return;
    }
    // Otherwise set title.
    $this->htmlHeadContexts['content_title'] = $title;
  }

  /**
   * Set page context - credential.
   *
   * @param array $credential_settings
   *   Credential settings array.
   */
  private function setContextCredential(array $credential_settings) {
    foreach (self::$credentialMapping as $credential_key => $tag_name) {
      if (isset($credential_settings[$credential_key])) {
        $this->htmlHeadContexts[$tag_name] = $credential_settings[$credential_key];
      }
    };
  }

  /**
   * Set page context - advanced.
   *
   * @param array $advanced_settings
   *   Advanced settings array.
   */
  private function setContextAdvanced(array $advanced_settings) {
    $bootstrap_mode = $advanced_settings['bootstrap_mode'] ?? 'auto';
    $replacement_mode = $advanced_settings['content_replacement_mode'];
    $cdf_version = $advanced_settings['cdf_version'] ?? SettingsHelper::CDF_VERSION_DEFAULT;
    if (SettingsHelper::isValidBootstrapMode($bootstrap_mode)) {
      $this->htmlHeadContexts['bootstrapMode'] = $bootstrap_mode;
    }
    if (SettingsHelper::isValidContentReplacementMode($replacement_mode)) {
      $this->htmlHeadContexts['contentReplacementMode'] = $replacement_mode;
    }
    if (SettingsHelper::isValidCdfVersion($cdf_version)) {
      $this->htmlHeadContexts['cdfVersion'] = $cdf_version;
    }
    if (!empty($advanced_settings['content_origins'])) {
      $content_origins = array_map('trim', explode("\n", trim($advanced_settings['content_origins'])));
      $content_origins = implode(',', $content_origins);
      if (!empty($content_origins)) {
        $this->htmlHeadContexts['content_origins'] = $content_origins;
      }
    }
  }

  /**
   * Set Node data.
   *
   * @param \Drupal\node\NodeInterface $node
   *   Node.
   */
  private function setNodeData(NodeInterface $node) {
    $this->htmlHeadContexts['content_type'] = $node->getType();
    $this->htmlHeadContexts['content_title'] = $node->getTitle();
    $this->htmlHeadContexts['published_date'] = $node->getCreatedTime();
    $this->htmlHeadContexts['post_id'] = $node->id();
    $this->htmlHeadContexts['content_uuid'] = $node->uuid();
    $this->htmlHeadContexts['page_type'] = 'node page';
  }

  /**
   * Set fields.
   *
   * @param \Drupal\node\NodeInterface $node
   *   Node.
   */
  private function setFields(NodeInterface $node) {
    $available_field_vocabulary_names = $this->getAvailableFieldVocabularyNames($node);
    $vocabulary_term_names = $this->getVocabularyTermNames($node);

    // Find Field Term names.
    foreach ($available_field_vocabulary_names as $page_context_name => $vocabulary_names) {
      $field_term_names = $this->getFieldTermNames($vocabulary_names, $vocabulary_term_names);
      // Only set when the value is a populated array
      // Empty arrays return as false in PHP.
      if (!empty($field_term_names)) {
        $this->htmlHeadContexts[$page_context_name] = implode(',', $field_term_names);
      }
    }
  }

  /**
   * Get available Fields and their vocabulary names within the node.
   *
   * @param \Drupal\node\NodeInterface $node
   *   Node.
   *
   * @return array
   *   Node's available Fields' Vocabularies names.
   */
  private function getAvailableFieldVocabularyNames(NodeInterface $node) {
    $available_field_vocabulary_names = [];
    $available_field_vocabulary_fields = [];

    // Regular field mapping.
    foreach ($this->fieldMappings as $page_context_name => $field_name) {
      if (!isset($node->{$field_name})) {
        continue;
      }
      // Add this field to the list of fields to parse with their corresponding
      // page context name;.
      if (!isset($available_field_vocabulary_fields[$field_name])) {
        $available_field_vocabulary_fields[$field_name] = [];
      }
      $available_field_vocabulary_fields[$field_name][] = $page_context_name;
    }
    // The following 3 mappings have all the same structure with different array
    // id's so we can merge them without conflict.
    $udf_mappings = array_merge($this->udfPersonMappings, $this->udfTouchMappings, $this->udfEventMappings);
    foreach ($udf_mappings as $page_context_name => $properties) {
      if (!isset($node->{$properties['value']})) {
        continue;
      }
      // Add this field to the list of fields to parse with their corresponding
      // page context name;.
      if (!isset($available_field_vocabulary_fields[$properties['value']])) {
        $available_field_vocabulary_fields[$properties['value']] = [];
      }
      $available_field_vocabulary_fields[$properties['value']][] = $page_context_name;
    }

    foreach ($available_field_vocabulary_fields as $field_name => $page_contexts) {
      $field_handler_settings = $node->{$field_name}->getSetting('handler_settings');
      $vocabulary_names = array_key_exists('target_bundles', $field_handler_settings) ? $field_handler_settings['target_bundles'] : [];
      foreach ($page_contexts as $page_context_name) {
        $available_field_vocabulary_names[$page_context_name] = $vocabulary_names;
      }
    }

    return $available_field_vocabulary_names;
  }

  /**
   * Get Vocabularies' Term names.
   *
   * @param \Drupal\node\NodeInterface $node
   *   Node.
   *
   * @return array
   *   Vocabularies' Term names.
   */
  private function getVocabularyTermNames(NodeInterface $node) {
    // Find the node's terms.
    $terms = $this->taxonomyTermStorage->getNodeTerms([$node->id()]);
    $node_terms = $terms[$node->id()] ?? [];

    // Find the term names.
    $vocabulary_term_names = [];
    foreach ($node_terms as $term) {
      $vocabulary_id = $term->bundle();
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
   *
   * @return array
   *   Field Term names.
   */
  private function getFieldTermNames(array $vocabulary_names, array $vocabulary_term_names) {
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
   * Get the render array for a JavaScript tag.
   *
   * @return array
   *   The render array
   */
  private function getJavaScriptTagRenderArray() {
    return [
      [
        '#tag' => 'script',
        '#attributes' => [
          'src' => $this->assetsUrl . '/' . self::LIFT_JS_FILENAME,
          'async' => TRUE,
        ],
      ],
      'acquia_lift_javascript',
    ];
  }

  /**
   * {@inheritdoc}
   */
  protected function populateHtmlHead(&$htmlHead) {
    parent::populateHtmlHead($htmlHead);

    // Attach Lift's JavaScript.
    $htmlHead[] = $this->getJavaScriptTagRenderArray();
  }

}
