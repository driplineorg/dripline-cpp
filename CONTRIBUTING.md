# Introduction

Thank you for considering contributing to dripline-cpp.

Following these guidelines helps to communicate that you respect the time of 
the developers managing and developing this open source project. In return, 
they should reciprocate that respect in addressing your issue, assessing changes, 
and helping you finalize your pull requests.

[source: [Hoodie](https://github.com/hoodiehq/hoodie/blob/master/CONTRIBUTING.md)]

## How you can contribute

There are many ways to contribute, from writing tutorials, improving the documentation, 
submitting bug reports and feature requests, or writing code which can be 
incorporated into dripline-cpp itself.

Please, don't use the issue tracker for questions about the dripline standard or 
support questions. 

# Ground Rules
This includes not just how to communicate with others (being respectful, considerate, etc) 
but also technical responsibilities (importance of testing, project dependencies, etc). 
Mention and link to your code of conduct, if you have one.

Responsibilities
* Create issues for any major changes and enhancements that you wish to make. Discuss things 
transparently and get community feedback.
* Ensure cross-platform compatibility for every change that's accepted. Windows, Mac, 
  Debian & Ubuntu Linux.
* Include tests and documentation for any new functionality.
* Ensure that code that goes into core meets all requirements in this checklist: 
  https://gist.github.com/audreyr/4feef90445b9680475f2
* Keep feature versions as small as possible, preferably one new feature per version.
* Be welcoming to newcomers and encourage diverse new contributors from all backgrounds. 
  See the [Code of Conduct](https://github.com/driplineorg/dripline-cpp/blob/master/CODE_OF_CONDUCT.md).

# Your First Contribution

Unsure where to begin contributing to Atom? You can start by looking through these beginner 
and help-wanted issues:
* Beginner issues - issues which should only require a few lines of code, and a test or two.
* Help wanted issues - issues which should be a bit more involved than beginner issues.

Working on your first Pull Request?  Check out these excellent resources: 
http://makeapullrequest.com/ and http://www.firsttimersonly.com/

# Getting started
### Give them a quick walkthrough of how to submit a contribution.
How you write this is up to you, but some things you may want to include:

* Let them know if they need to sign a CLA, agree to a DCO, or get any other legal stuff out of the way
* If tests are required for contributions, let them know, and explain how to run the tests
* If you use anything other than GitHub to manage issues (ex. JIRA or Trac), let them know which tools they’ll need to contribute

For something that is bigger than a one or two line fix:

1. Create your own fork of the code
2. Do the changes in your fork
3. If you like the change and think the project could use it:
    * Be sure you have followed the code style for the project.
    * Send a pull request.

### If you have a different process for small or "obvious" fixes, let them know.

> Small contributions such as fixing spelling errors, where the content is small enough to not be considered intellectual property, can be submitted by a contributor as a patch, without a CLA.
>
As a rule of thumb, changes are obvious fixes if they do not introduce any new functionality or creative thinking. As long as the change does not affect functionality, some likely examples include the following:
* Spelling / grammar fixes
* Typo correction, white space and formatting changes
* Comment clean up
* Bug fixes that change default return values or error codes stored in constants
* Adding logging messages or debugging output
* Changes to ‘metadata’ files like .gitignore, CMakeLists files, etc.


# How to report a bug
### Explain security disclosures first!
If you find a security vulnerability, do NOT open an issue.  Send a private Slack message to @Noah or @Ben LaRoque.

In order to determine whether you are dealing with a security issue, ask yourself these two questions:
* Can I access something that's not mine, or something I shouldn't have access to?
* Can I disable something for other people?

If the answer to either of those two questions are "yes", then you're probably dealing with a 
security issue. Note that even if you answer "no" to both questions, you may still be dealing 
with a security issue, so if you're unsure, just ask with a private Slack message.

### How to file a bug report.
 When filing an issue, make sure to answer these five questions:

1. What operating system and compiler are you using?
2. What did you do?
3. What did you expect to see?
4. What did you see instead?

General questions should be posted on the driplineorg Slack workspace, 
on the #dripline-cpp channel.

# How to suggest a feature or enhancement

If you find yourself wishing for a feature that doesn't exist in dripline-cpp, 
you are probably not alone. There are bound to be others out there with similar needs. 
Many of the features that dripline-cpp has today have been added because our users 
saw the need. Open an issue on our issues list on GitHub that describes the feature 
you would like to see, why you need it, and how it should work.

# Code review process
### Explain how a contribution gets accepted after it’s been submitted.
Who reviews it? Who needs to sign off before it’s accepted? When should a contributor expect to hear from you? How can contributors get commit access, if at all?

> The core team looks at Pull Requests on a regular basis in a weekly triage meeting that we hold in a public Google Hangout. The hangout is announced in the weekly status updates that are sent to the puppet-dev list. Notes are posted to the Puppet Community community-triage repo and include a link to a YouTube recording of the hangout.
> After feedback has been given we expect responses within two weeks. After two weeks we may close the pull request if it isn't showing any activity.

[source: [Puppet](https://github.com/puppetlabs/puppet/blob/master/CONTRIBUTING.md#submitting-changes)] **Need more inspiration?** [1] [Meteor](https://meteor.hackpad.com/Responding-to-GitHub-Issues-SKE2u3tkSiH ) [2] [Express.js](https://github.com/expressjs/express/blob/master/Contributing.md#becoming-a-committer)

# Community

You can chat with the dripline-cpp team on the #dripline-cpp channel of the driplineorg 
Slack workspace.  You can rquest access to the workspace by contacting @nsoblath on GitHub.


# BONUS: Code, commit message and labeling conventions
These sections are not necessary, but can help streamline the contributions you receive.

### Explain your preferred style for code, if you have any.

**Need inspiration?** [1] [Requirejs](http://requirejs.org/docs/contributing.html#codestyle) [2] [Elasticsearch](https://github.com/elastic/elasticsearch/blob/master/CONTRIBUTING.md#contributing-to-the-elasticsearch-codebase)

### Explain if you use any commit message conventions.

**Need inspiration?** [1] [Angular](https://github.com/angular/material/blob/master/.github/CONTRIBUTING.md#submit) [2] [Node.js](https://github.com/nodejs/node/blob/master/CONTRIBUTING.md#step-3-commit)

### Explain if you use any labeling conventions for issues.

**Need inspiration?** [1] [StandardIssueLabels](https://github.com/wagenet/StandardIssueLabels#standardissuelabels) [2] [Atom](https://github.com/atom/atom/blob/master/CONTRIBUTING.md#issue-and-pull-request-labels)