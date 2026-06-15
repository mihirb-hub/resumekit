# Install

This repo is meant to be used by an agent, not just read manually.

## Goal

Get the agent to:

1. read the files in this repo
2. guide the user through onboarding
3. collect project-level context from important repos
4. generate Jake-style LaTeX resumes and PDFs later

## Recommended Setup

Use one folder as your main resume workspace. That workspace is where onboarding should happen.

Inside that workspace, the agent should create and maintain:

```text
.resume-system/
resume-source/
job-postings/
output/
```

## Agent Usage Pattern

The intended flow is:

1. agent reads `commands/initialize-resume-system.md`
2. user adds base files into the resume workspace
3. agent builds the reusable context layer
4. user opens important project repos one by one
5. agent reads `commands/collect-project-context.md` inside those repos
6. repo context gets saved back into the main resume workspace
7. user returns to the main workspace
8. agent reads `commands/optimize-resume.md` for normal tailoring

## What the User Needs to Provide

The agent should explicitly ask the user for:

- current resume
- old tailored resumes
- transcript or coursework list
- brag doc or achievement notes
- project summaries or writeups
- portfolio copy
- strong project repos

## Important Behavior

- initialization should be guided and step-by-step
- project-context collection should be run inside real repos
- optimization should be a separate, lighter command after context is already collected
- resume output should default to Jake-style LaTeX
- PDF should be compiled by default when LaTeX tooling is available

## Positioning Philosophy

This system is meant to sell the user hard, but still stay tethered to reality.

Allowed:

- surfacing real but omitted skills
- using repo evidence to strengthen bullets
- using classwork, side projects, research, or smaller experience when it truthfully helps match the JD
- aggressively reframing adjacent real experience

Not allowed:

- inventing tools, frameworks, or languages the user did not actually use
- replacing one stack with another just to satisfy the JD if that would be false
- inventing metrics or ownership

## Next File to Read

After reading this file, the agent should read:

- `commands/initialize-resume-system.md`
