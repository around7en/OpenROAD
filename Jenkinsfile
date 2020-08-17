pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh './jenkins/install.sh'
      }
    }
    stage('Tests') {
      failFast true
      parallel {
        stage('gcd_nangate45') {
          steps {
	    sh './test/regression gcd_nangate45'
	  }
        }
        stage('Unit tests') {
	  steps {
            sh './test/regression'
	  }
        }
        stage('aes_nangate45') {
          steps {
	    sh './test/regression aes_nangate45'
	  }
        }
        stage('tinyRocket_nangate45') {
          steps {
	    sh './test/regression tinyRocket_nangate45'
	  }
        }
        stage('gcd_sky130') {
          steps {
	    sh './test/regression gcd_sky130'
	  }
        }
        stage('aes_sky130') {
          steps {
	    sh './test/regression aes_sky130'
	  }
        }
        stage('ibex_sky130') {
          steps {
	    sh './test/regression ibex_sky130'
	  }
        }
      }
    }
  }
}
