# badge2023 Documentation
This is a place to store public facing documentation.
This uses a static site generator [eleventy](https://www.11ty.dev/) that builds a documentation site to gh-pages.

## Adding Content

### Pages
You can add markdown files in the [./content/pages/](./content/pages/) directory.
These will show on the site as pages.

To get the page to link in the nav, you will need to include [frontmatter](https://www.11ty.dev/docs/data-frontmatter/) in the markdown file that looks something like this:

```md
---
title: Hello, World
date: Last Modified 
permalink: /hello_world.html
eleventyNavigation:
  key: hello
  order: 0
  title: Hello, World!
---

Hello, World!
```

### Images
Add images to the `web/user_docs/content/pages/images/` dir.
Files in this directory get passed through to this directory in the build: `/badge2023/images/`

in the markdown files, reference the images from a relative path.
> e.g. `images/someImage.jpg`

## Running the site locally
You will need to install node (probably something version 12 or higher) and npm.

Most package managers have an ancient version of node that probably won't work.
The easiest way to install node and npm is with the Node Version Manager (nvm).

### Node Version Manager
Follow these instructions
https://github.com/nvm-sh/nvm#install--update-script

You should now be able to run node and npm.
```
npm -v
```
