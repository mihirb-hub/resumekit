# Collect Project Context

You are inside a specific project repository. Your job is to extract as much useful resume context as possible from this repo without inventing personal ownership.

## Goal

Build a reusable project-context file that can later be used to strengthen resume bullets.

This command should:

1. inspect the repo
2. infer stack and architecture from files
3. ask the user what they personally built or owned
4. save the result into the main resume workspace

## Guided Behavior

The user may not know what this command is doing. Explain it clearly:

- `I am scanning this repo to understand the real stack and technical depth behind this project.`
- `After that, I will ask what you personally owned so I do not overclaim.`
- `Then I will save a reusable context file for future resume tailoring.`

## Step 1: Inspect the Repo

Read the highest-signal files first:

- `README*`
- `package.json`
- `requirements.txt`
- `pyproject.toml`
- `Cargo.toml`
- `pom.xml`
- `build.gradle*`
- `Dockerfile*`
- `docker-compose*`
- `.github/workflows/*`
- deployment configs
- infra configs
- database schema files
- API specs
- top-level source directories

Extract:

- languages
- frameworks
- databases
- cloud platforms
- containers / orchestration
- CI/CD
- testing frameworks
- analytics / observability
- auth / security
- AI / ML libraries or APIs
- architecture patterns
- likely product surface

## Step 2: Ask What the User Personally Owned

Ask the user to clarify:

- what they personally built
- what they materially contributed to
- what they only touched lightly
- what was team-wide but not theirs
- any metrics, scale, outcomes, or constraints worth saving

## Step 3: Save the Context File

Write:

`.resume-system/project-context/[repo-name].md`

The file should distinguish clearly between:

- inferred repo-level stack
- user-confirmed ownership

Do not convert `the repo contains X` into `the user built X` unless supported.

## Step 4: Update the Project Context Index

Append to:

`.resume-system/PROJECT_CONTEXT_INDEX.md`

Include:

- repo name
- saved context path
- strongest technologies surfaced
- strongest resume angles
- whether ownership was confirmed by the user

## Step 5: Tell the User What to Do Next

Close with direct guidance:

1. `If you have more strong repos, run this command in those too.`
2. `When you are done, go back to your main resume workspace.`
3. `Then run the main resume optimization command there.`
