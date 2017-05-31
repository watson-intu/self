# Release process

 * code: https://github.com/watson-developer-cloud/c++-sdk
 * maven: http://search.maven.org/#search%7Cga%7C1%7Ca%3A%22java-sdk%22

## Uploading artifacts to the Central Repository

We use Sonatype as repository manager. It's used as the input channel for the Central Repository running the Sonatype Open Source Repository Hosting (OSSRH) service.

### Prerequisites

If you are not familiar with Sonatype, releasing artifacts to Sonatype, or working with PGP signatures, see the following information:  
 * [Sonatype](http://central.sonatype.org/pages/apache-maven.html)
 * [Releasing artifacts to Sonatype]( http://kirang89.github.io/blog/2013/01/20/uploading-your-jar-to-maven-central/)
 * [Working with PGP signatures]( http://central.sonatype.org/pages/working-with-pgp-signatures.html)


### Release steps

  1. Ensure that `pom.xml` and `build.gradle` have the same version number. In the `pom.xml` file the version should end with -SNAPSHOT

  2. With the SCM connection configured correctly you can perform a release deployment to OSSRH (Staging) by issuing this command:

    `mvn release:clean release:prepare`

    You must answer prompts for versions and tags. That will tag and commit a new version into your repository automatically.

  3. Deploy to the Maven Central Repository by issuing the following command:

    `mvn release:perform`

    This command deploys to OSSRH and releases to the Central Repository in one step.