// The pipeline for Jenkins (declarative syntax). Every step calls tools/ci.sh, so a developer runs the same commands.
pipeline {
    agent none
    options {
        timeout(time: 30, unit: 'MINUTES')
        buildDiscarder(logRotator(numToKeepStr: '30', artifactNumToKeepStr: '10'))
        disableConcurrentBuilds(abortPrevious: true)
        timestamps()
        // The history and the tags come from the checkout in the Git step below, not from a shallow clone.
        skipDefaultCheckout(true)
    }
    environment {
        VERBOSE = '1'
    }
    stages {
        stage('Static') {
            agent {
                docker { image 'registry.example.com/team/sensorfw-ci:1' }
            }
            steps {
                checkout scmGit(branches: scm.branches, userRemoteConfigs: scm.userRemoteConfigs,
                                extensions: [cloneOption(shallow: false, noTags: false)])
                sh 'tools/ci.sh static meta'
            }
        }
        stage('Test') {
            matrix {
                axes {
                    axis { name 'STAGE'; values 'host', 'asan', 'coverage', 'm4' }
                }
                stages {
                    stage('Run') {
                        agent {
                            docker { image 'registry.example.com/team/sensorfw-ci:1' }
                        }
                        steps {
                            checkout scmGit(branches: scm.branches, userRemoteConfigs: scm.userRemoteConfigs,
                                            extensions: [cloneOption(shallow: false, noTags: false)])
                            sh "tools/ci.sh ${STAGE} meta"
                        }
                        post {
                            always {
                                junit allowEmptyResults: true, testResults: 'build/reports/junit-*.xml'
                                archiveArtifacts artifacts: 'build/reports/**', allowEmptyArchive: true
                                stash name: "reports-${STAGE}", includes: 'build/reports/**', allowEmpty: true
                            }
                        }
                    }
                }
            }
        }
        // The tests on the board run on an agent of ours that has the board attached, and only on the main branch.
        stage('Board') {
            when { branch 'main' }
            agent { label 'board-m4' }
            environment {
                BOARD_TTY = '/dev/ttyACM0'
            }
            steps {
                checkout scmGit(branches: scm.branches, userRemoteConfigs: scm.userRemoteConfigs,
                                extensions: [cloneOption(shallow: false, noTags: false)])
                sh 'tools/ci.sh board meta'
            }
            post {
                always {
                    junit allowEmptyResults: true, testResults: 'build/reports/junit-board.xml'
                    archiveArtifacts artifacts: 'build/reports/**', allowEmptyArchive: true
                }
            }
        }
        // The reports of the test stages go to the dashboard, and only from the main branch. The token is a credential.
        stage('Publish') {
            when { branch 'main' }
            agent {
                docker { image 'registry.example.com/team/sensorfw-ci:1' }
            }
            environment {
                DASHBOARD_URL = 'https://dashboard.example.com'
                SFW_TOKEN = credentials('sfw-token')
            }
            steps {
                checkout scmGit(branches: scm.branches, userRemoteConfigs: scm.userRemoteConfigs,
                                extensions: [cloneOption(shallow: false, noTags: false)])
                script {
                    for (name in ['host', 'asan', 'coverage', 'm4']) {
                        unstash "reports-${name}"
                    }
                }
                sh 'tools/ci.sh publish'
            }
        }
    }
}
