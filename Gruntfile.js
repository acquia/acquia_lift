module.exports = function(grunt) {
  require('time-grunt')(grunt);

  var reportABjs = [
        'src/js/Rickshaw.Graph.Axis.TimeElement.js',
        'src/js/Rickshaw.Graph.Axis.LabeledY.js',
        'src/js/Rickshaw.Graph.ClickDetail.js',
        'src/js/Rickshaw.Graph.TableLegend.js',
        'src/js/acquia_lift.report.ab.js'
      ];

  // Project configuration.
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    autoprefixer: {
      css: {
        src: 'css/**/*.css'
      }
    },
    concat: {
      options: {
        sourceMap: true,
        separator: "\n"
      },
      reportAB: {
        src: reportABjs,
        dest: 'js/acquia_lift.report.ab.js'
      }
    },
    concurrent: {
      all: ['style', 'script'],
    },
    sass: {
      dist: {
        options: {
          style: 'expanded'
        },
        files: {'css/acquia_lift.report.ab.css': 'src/css/acquia_lift.report.ab.scss'}
      }
    },
    watch: {
      sass: {
        files: 'src/css/**/*.scss',
        tasks: ['style']
      },
      scripts: {
        files: 'src/js/**/*.js',
        tasks: ['script']
      },
      gruntfile: {
        files: 'Gruntfile.js',
        tasks: ['default']
      }
    }
  });

  // Load the plugin that provides the "uglify" task.
  grunt.loadNpmTasks('grunt-autoprefixer');
  grunt.loadNpmTasks('grunt-concurrent');
  grunt.loadNpmTasks('grunt-contrib-concat');
  grunt.loadNpmTasks('grunt-contrib-sass');
  grunt.loadNpmTasks('grunt-contrib-watch');

  // Default task(s).
  grunt.registerTask('default', ['concurrent:all']);
  grunt.registerTask('style', ['sass', 'autoprefixer']);
  grunt.registerTask('script', ['concat']);

};
